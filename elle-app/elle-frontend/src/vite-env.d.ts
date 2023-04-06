/// <reference types="vite/client" />
vite: {
    resolve: {
      alias: {
        $houdini: path.resolve('.', '$houdini'),
      },
    },
    server: {
      fs: {
        // Allow serving files from one level up to the project root
        allow: ['..'],
      },
    },
  },
  
  