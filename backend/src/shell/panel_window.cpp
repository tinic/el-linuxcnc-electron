#include "panel_window.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <csignal>

#include <glib-unix.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

// GLib, GTK and WebKit may only be included through their umbrella headers (their inner headers
// #error otherwise), so include-cleaner cannot be satisfied for their symbols in this file.
// NOLINTBEGIN(misc-include-cleaner)

namespace elle::shell {

namespace {

constexpr const char* scheme = "elle";
constexpr std::string_view panel_url = "elle://panel/index.html";
constexpr int default_width = 1366;
constexpr int default_height = 768;

struct MimeType {
    std::string_view extension{};
    std::string_view type{};
};

constexpr auto mime_types = std::to_array<MimeType>({
    {.extension = ".html", .type = "text/html"},
    {.extension = ".js", .type = "text/javascript"},
    {.extension = ".css", .type = "text/css"},
    {.extension = ".json", .type = "application/json"},
    {.extension = ".svg", .type = "image/svg+xml"},
    {.extension = ".png", .type = "image/png"},
    {.extension = ".ttf", .type = "font/ttf"},
    {.extension = ".woff2", .type = "font/woff2"},
});

auto mime_type(const std::filesystem::path& file) -> std::string_view
{
    const std::string extension = file.extension().string();
    const auto* const match = std::ranges::find(mime_types, extension, &MimeType::extension);
    return match == mime_types.end() ? std::string_view{"application/octet-stream"} : match->type;
}

/// Resolves a request path inside `root`, refusing anything that would leave it.
auto resolve(const std::filesystem::path& root, std::string_view request_path)
    -> std::optional<std::filesystem::path>
{
    const std::filesystem::path relative =
        std::filesystem::path{request_path}.relative_path().lexically_normal();
    if (relative.empty() || *relative.begin() == "..") {
        return std::nullopt;
    }
    return root / relative;
}

auto read_file(const std::filesystem::path& file) -> std::optional<std::string>
{
    std::ifstream stream{file, std::ios::binary};
    if (!stream.is_open()) {
        return std::nullopt;
    }
    return std::string{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}

} // namespace

struct PanelWindow::Impl {
    PanelOptions options{};
    PanelCallbacks callbacks{};
    GtkWidget* window{};
    WebKitWebView* view{};
    guint refresh_source{};
    std::array<guint, 3> signal_sources{};

    static auto on_message(WebKitUserContentManager* /*manager*/, WebKitJavascriptResult* result,
                           gpointer data) -> void
    {
        auto* self = static_cast<Impl*>(data);
        JSCValue* value = webkit_javascript_result_get_js_value(result);
        gchar* text = jsc_value_to_string(value);
        if (text != nullptr) {
            self->callbacks.message(text);
            g_free(text);
        }
    }

    static auto on_refresh(gpointer data) -> gboolean
    {
        static_cast<Impl*>(data)->callbacks.refresh();
        return G_SOURCE_CONTINUE;
    }

    /// Runs on the UI thread (GLib turns the signal into a main-loop event), never in the handler.
    static auto on_terminate(gpointer data) -> gboolean
    {
        static_cast<Impl*>(data)->callbacks.terminate();
        return G_SOURCE_CONTINUE;
    }

    static auto on_scheme(WebKitURISchemeRequest* request, gpointer data) -> void
    {
        const auto* self = static_cast<Impl*>(data);
        const auto file = resolve(self->options.ui_directory,
                                  webkit_uri_scheme_request_get_path(request));
        const auto content = file.has_value() ? read_file(*file) : std::nullopt;
        if (!content.has_value()) {
            GError* error = g_error_new_literal(G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                                                webkit_uri_scheme_request_get_uri(request));
            webkit_uri_scheme_request_finish_error(request, error);
            g_error_free(error);
            return;
        }
        GBytes* bytes = g_bytes_new(content->data(), content->size());
        GInputStream* stream = g_memory_input_stream_new_from_bytes(bytes);
        const std::string type{mime_type(*file)};
        webkit_uri_scheme_request_finish(request, stream, static_cast<gint64>(content->size()),
                                         type.c_str());
        g_object_unref(stream);
        g_bytes_unref(bytes);
    }

    // NOLINTNEXTLINE(readability-function-size): the parameter list is GTK's signal signature.
    static auto on_context_menu(WebKitWebView* /*view*/, WebKitContextMenu* /*menu*/,
                                GdkEvent* /*event*/, WebKitHitTestResult* /*hit*/,
                                gpointer /*data*/) -> gboolean
    {
        return TRUE; // a touch panel has no use for "Reload" or "Inspect"
    }

    static auto on_destroy(GtkWidget* /*widget*/, gpointer /*data*/) -> void
    {
        gtk_main_quit();
    }
};

auto initialize_gtk() -> bool
{
    return gtk_init_check(nullptr, nullptr) != FALSE;
}

PanelWindow::PanelWindow(PanelOptions options, PanelCallbacks callbacks)
    : impl_{std::make_unique<Impl>()}
{
    Impl& impl = *impl_;
    impl.options = std::move(options);
    impl.callbacks = std::move(callbacks);

    WebKitWebContext* context = webkit_web_context_get_default();
    webkit_web_context_register_uri_scheme(context, scheme, &Impl::on_scheme, &impl,
                                           nullptr);
    WebKitSecurityManager* security = webkit_web_context_get_security_manager(context);
    webkit_security_manager_register_uri_scheme_as_secure(security, scheme);
    webkit_security_manager_register_uri_scheme_as_cors_enabled(security, scheme);

    WebKitUserContentManager* content = webkit_user_content_manager_new();
    g_signal_connect(content, "script-message-received::elle", G_CALLBACK(&Impl::on_message),
                     &impl);
    webkit_user_content_manager_register_script_message_handler(content, "elle");

    impl.view = WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(content));
    WebKitSettings* settings = webkit_web_view_get_settings(impl.view);
    webkit_settings_set_enable_developer_extras(settings,
                                                impl.options.dev_url.has_value() ? TRUE : FALSE);
    webkit_settings_set_enable_back_forward_navigation_gestures(settings, FALSE);
    g_signal_connect(impl.view, "context-menu", G_CALLBACK(&Impl::on_context_menu), &impl);

    impl.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(impl.window), "Elle");
    gtk_window_set_default_size(GTK_WINDOW(impl.window), default_width, default_height);
    gtk_container_add(GTK_CONTAINER(impl.window), GTK_WIDGET(impl.view));
    g_signal_connect(impl.window, "destroy", G_CALLBACK(&Impl::on_destroy), &impl);
    if (!impl.options.windowed) {
        gtk_window_set_decorated(GTK_WINDOW(impl.window), FALSE);
        gtk_window_fullscreen(GTK_WINDOW(impl.window));
    }

    const std::string url = impl.options.dev_url.value_or(std::string{panel_url});
    webkit_web_view_load_uri(impl.view, url.c_str());

    impl.refresh_source = g_timeout_add(static_cast<guint>(refresh_interval.count()),
                                        &Impl::on_refresh, &impl);
    constexpr auto terminating = std::to_array({SIGINT, SIGTERM, SIGHUP});
    std::ranges::transform(terminating, impl.signal_sources.begin(), [&impl](int signal_number) {
        return g_unix_signal_add(signal_number, &Impl::on_terminate, &impl);
    });
}

PanelWindow::~PanelWindow()
{
    if (impl_->refresh_source != 0) {
        g_source_remove(impl_->refresh_source);
    }
    for (const guint source : impl_->signal_sources) {
        if (source != 0) {
            g_source_remove(source);
        }
    }
}

auto PanelWindow::post(std::string_view json) -> void
{
    // JSON is a valid JavaScript expression, so the message arrives as an object.
    const std::string script = std::format("window.elleReceive?.({});", json);
    webkit_web_view_evaluate_javascript(impl_->view, script.c_str(),
                                        static_cast<gssize>(script.size()), nullptr, nullptr,
                                        nullptr, nullptr, nullptr);
}

auto PanelWindow::run() -> void
{
    gtk_widget_show_all(impl_->window);
    gtk_main();
}

auto PanelWindow::quit() -> void
{
    gtk_widget_destroy(impl_->window); // its "destroy" handler ends the main loop
}

} // namespace elle::shell

// NOLINTEND(misc-include-cleaner)
