<script setup lang="ts">
// What each canned-cycle field does, drawn on a half-section of the work (chuck on the left, work
// face on the right, centreline at the bottom; proportions exaggerated for legibility). The field
// being edited or whose label was tapped is highlighted.
defineProps<{ kind: 'turning' | 'threading'; highlight: string | null }>()
</script>

<template>
  <svg
    v-if="kind === 'turning'"
    class="diagram"
    viewBox="0 0 420 330"
    role="img"
    aria-label="Turning cycle: what the fields mean"
  >
    <defs>
      <pattern id="removed" width="7" height="7" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">
        <line x1="0" y1="0" x2="0" y2="7" class="hatch" />
      </pattern>
      <marker id="arrow" viewBox="0 0 8 8" refX="7" refY="4" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
        <path d="M0,0 L8,4 L0,8 z" class="arrowhead" />
      </marker>
    </defs>

    <!-- chuck, finished part, material the cycle removes -->
    <rect x="14" y="58" width="26" height="242" class="chuck" />
    <polygon points="40,70 150,70 150,145 330,170 330,300 40,300" class="part" />
    <polygon points="150,70 330,70 330,170 150,145" class="removed" />
    <line x1="10" y1="300" x2="410" y2="300" class="centre" />
    <text x="44" y="292" class="note">centreline</text>

    <!-- tool at the stock -->
    <polygon points="347,69 372,52 380,62" class="tool" />
    <text x="356" y="44" class="note" text-anchor="middle">tool X = stock</text>

    <!-- roughing passes (Step) and the final pass (Final, Spring) -->
    <g :class="{ on: highlight === 'step_down' }">
      <line x1="345" y1="97" x2="150" y2="70" class="pass" />
      <line x1="345" y1="122" x2="150" y2="95" class="pass" />
      <line x1="345" y1="147" x2="150" y2="120" class="pass" />
      <line x1="345" y1="162" x2="150" y2="135" class="pass" />
      <line x1="362" y1="95" x2="362" y2="120" class="dim" marker-start="url(#arrow)" marker-end="url(#arrow)" />
      <text x="368" y="112" class="label">Cut depth</text>
    </g>
    <g :class="{ on: highlight === 'final_step_down' || highlight === 'spring_passes' }">
      <line x1="345" y1="172" x2="150" y2="145" class="final" />
    </g>
    <g :class="{ on: highlight === 'final_step_down' }">
      <line x1="362" y1="160" x2="362" y2="170" class="dim" />
      <line x1="356" y1="160" x2="368" y2="160" class="dim" />
      <line x1="356" y1="170" x2="368" y2="170" class="dim" />
      <text x="368" y="160" class="label">Finish</text>
    </g>
    <g :class="{ on: highlight === 'spring_passes' }">
      <text x="240" y="196" class="label" text-anchor="middle">Spring: final pass repeated</text>
    </g>

    <!-- Target: finished size at the face -->
    <g :class="{ on: highlight === 'target' }">
      <line x1="330" y1="170" x2="398" y2="170" class="ext" />
      <line x1="392" y1="300" x2="392" y2="172" class="dim" marker-start="url(#arrow)" marker-end="url(#arrow)" />
      <text x="400" y="240" class="label" text-anchor="middle" transform="rotate(-90 400 240)">Target ⌀</text>
    </g>

    <!-- Length: from the face (Z0) into the work -->
    <g :class="{ on: highlight === 'length' }">
      <line x1="150" y1="145" x2="150" y2="324" class="ext" />
      <line x1="330" y1="300" x2="330" y2="324" class="ext" />
      <line x1="152" y1="318" x2="328" y2="318" class="dim" marker-start="url(#arrow)" marker-end="url(#arrow)" />
      <text x="240" y="313" class="label" text-anchor="middle">Length</text>
      <text x="335" y="327" class="note">Z0 (face)</text>
    </g>

    <!-- Taper -->
    <g :class="{ on: highlight === 'taper_angle_deg' }">
      <line x1="150" y1="145" x2="215" y2="145" class="ext" />
      <path d="M 205 145 A 55 55 0 0 1 204.5 152.6" class="dim" />
      <text x="212" y="140" class="label">Taper ∠</text>
    </g>

    <!-- Feed: tool travel per spindle revolution -->
    <g :class="{ on: highlight === 'feed_rate' }">
      <line x1="320" y1="22" x2="210" y2="22" class="dim" marker-end="url(#arrow)" />
      <text x="265" y="15" class="label" text-anchor="middle">Feed per revolution</text>
    </g>

    <!-- legend -->
    <line x1="50" y1="16" x2="72" y2="16" class="pass" />
    <text x="78" y="20" class="note">roughing passes</text>
    <line x1="50" y1="32" x2="72" y2="32" class="final" />
    <text x="78" y="36" class="note">final pass</text>
  </svg>

  <svg v-else class="diagram" viewBox="0 0 420 330" role="img" aria-label="Threading cycle: what the fields mean">
    <defs>
      <marker id="arrow-t" viewBox="0 0 8 8" refX="7" refY="4" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
        <path d="M0,0 L8,4 L0,8 z" class="arrowhead" />
      </marker>
    </defs>

    <!-- chuck, work with the thread cut into it -->
    <rect x="14" y="98" width="26" height="202" class="chuck" />
    <polygon
      points="40,110 120,110 135,150 150,110 165,150 180,110 195,150 210,110 225,150 240,110 255,150 270,110 285,150 300,110 300,300 40,300"
      class="part"
    />
    <line x1="10" y1="300" x2="410" y2="300" class="centre" />
    <text x="44" y="292" class="note">centreline</text>

    <!-- where the operator puts the tool, and where passes start -->
    <line x1="300" y1="60" x2="300" y2="324" class="ext" />
    <text x="304" y="327" class="note">Z0 (face)</text>
    <polygon points="360,109 352,92 368,92" class="tool" />
    <text x="414" y="86" class="note" text-anchor="end">passes start here</text>

    <!-- Pitch -->
    <g :class="{ on: highlight === 'pitch' }">
      <line x1="240" y1="110" x2="240" y2="96" class="ext" />
      <line x1="270" y1="110" x2="270" y2="96" class="ext" />
      <line x1="242" y1="99" x2="268" y2="99" class="dim" marker-start="url(#arrow-t)" marker-end="url(#arrow-t)" />
      <text x="255" y="92" class="label" text-anchor="middle">Pitch</text>
    </g>

    <!-- XDepth (radial) and ZDepth (sideways): together the infeed direction -->
    <g :class="{ on: highlight === 'depth' }">
      <line x1="300" y1="150" x2="336" y2="150" class="ext" />
      <line x1="330" y1="112" x2="330" y2="148" class="dim" marker-start="url(#arrow-t)" marker-end="url(#arrow-t)" />
      <text x="336" y="134" class="label">Depth</text>
    </g>
    <g :class="{ on: highlight === 'infeed_deg' || highlight === 'depth' }">
      <line x1="226" y1="58" x2="216" y2="104" class="infeed" marker-end="url(#arrow-t)" />
    </g>
    <g :class="{ on: highlight === 'infeed_deg' }">
      <text x="232" y="66" class="label">Infeed ∠</text>
    </g>

    <!-- Angle: tapered thread -->
    <g :class="{ on: highlight === 'taper_deg' }">
      <line x1="300" y1="110" x2="120" y2="100" class="taper" />
      <text x="124" y="92" class="label">Taper ∠</text>
    </g>

    <!-- ZEnd: thread length from the tool's Z -->
    <g :class="{ on: highlight === 'length' }">
      <line x1="120" y1="110" x2="120" y2="324" class="ext" />
      <line x1="122" y1="318" x2="298" y2="318" class="dim" marker-start="url(#arrow-t)" marker-end="url(#arrow-t)" />
      <text x="210" y="313" class="label" text-anchor="middle">Length</text>
    </g>

    <!-- end of each pass: the run-out pulls out at an angle, finishing at the thread end; then the
         retract clears for the rapid return -->
    <g :class="{ on: highlight === 'run_out' }">
      <line x1="150" y1="150" x2="120" y2="110" class="pullout" />
      <line x1="150" y1="150" x2="150" y2="172" class="ext" />
      <line x1="122" y1="168" x2="148" y2="168" class="dim" marker-start="url(#arrow-t)" marker-end="url(#arrow-t)" />
      <text x="62" y="186" class="label">Run-out</text>
    </g>
    <g :class="{ on: highlight === 'retract' }">
      <line x1="120" y1="108" x2="120" y2="44" class="dim" marker-end="url(#arrow-t)" />
      <text x="58" y="70" class="label">Retract</text>
    </g>
    <line x1="124" y1="40" x2="360" y2="40" class="rapid" />
    <text x="226" y="33" class="note" text-anchor="middle">rapid back for the next pass</text>

    <!-- cut depth per pass: 1stCut, then x Mult each pass, never below MinCut; Spring at full depth -->
    <g transform="translate(150 196)">
      <text x="0" y="0" class="note on-part">depth per pass</text>
      <g :class="{ on: highlight === 'first_cut' }">
        <rect x="0" y="8" width="12" height="40" class="bar" />
        <text x="6" y="62" class="label small" text-anchor="middle">First</text>
      </g>
      <g :class="{ on: highlight === 'cut_factor' }">
        <rect x="20" y="8" width="12" height="32" class="bar" />
        <rect x="36" y="8" width="12" height="26" class="bar" />
        <text x="34" y="76" class="label small" text-anchor="middle">×Factor</text>
      </g>
      <g :class="{ on: highlight === 'min_cut' }">
        <rect x="56" y="8" width="12" height="20" class="bar" />
        <rect x="72" y="8" width="12" height="20" class="bar" />
        <text x="70" y="62" class="label small" text-anchor="middle">Min</text>
      </g>
      <g :class="{ on: highlight === 'spring_cuts' }">
        <rect x="92" y="8" width="12" height="20" class="bar spring" />
        <text x="98" y="76" class="label small" text-anchor="middle">Spring</text>
      </g>
    </g>
  </svg>
</template>

<style scoped>
.diagram {
  width: 100%;
  height: 100%;
  font: inherit;
}

.chuck {
  fill: var(--control);
  stroke: var(--text-faint);
}

.part {
  fill: var(--control-active);
  stroke: var(--text-dim);
  stroke-width: 1.2;
}

.removed {
  fill: url(#removed);
  stroke: var(--text-faint);
  stroke-dasharray: 4 3;
}

.hatch {
  stroke: var(--text-faint);
  stroke-width: 1.2;
}

.centre {
  stroke: var(--text-faint);
  stroke-dasharray: 14 4 3 4;
}

.tool {
  fill: var(--warn);
}

.arrowhead {
  fill: var(--text-dim);
}

.dim,
.pass,
.final,
.ext,
.taper,
.infeed,
.pullout,
.rapid {
  fill: none;
  stroke: var(--text-dim);
  stroke-width: 1.2;
}

.ext {
  stroke: var(--text-faint);
  stroke-width: 0.8;
}

.pass {
  stroke-dasharray: 5 4;
}

.final {
  stroke: var(--text);
  stroke-width: 2;
}

.taper {
  stroke-dasharray: 2 3;
}

.infeed {
  stroke-width: 1.6;
}

.pullout {
  stroke: var(--text);
  stroke-width: 1.6;
}

.rapid {
  stroke: var(--text-faint);
  stroke-dasharray: 6 4;
}

.bar {
  fill: var(--control-hover);
  stroke: var(--text-faint);
}

.bar.spring {
  fill: var(--control-active);
}

.label {
  fill: var(--text-dim);
  font-size: 14px;
  font-weight: 700;
}

.label.small {
  font-size: 11px;
}

.note {
  fill: var(--text-faint);
  font-size: 11px;
}

.note.on-part {
  fill: var(--text-dim);
}

/* The field being edited, or whose label was tapped. */
.on .label {
  fill: var(--entry);
}

.on .dim,
.on .pass,
.on .final,
.on .taper,
.on .infeed,
.on .pullout {
  stroke: var(--entry);
  stroke-width: 2.2;
}

.on .bar {
  fill: var(--entry);
  stroke: var(--entry);
}
</style>
