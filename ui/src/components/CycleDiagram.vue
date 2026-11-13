<script setup lang="ts">
// What each canned-cycle field does, drawn on a half-section of the work (chuck on the left, work
// face on the right, centreline at the bottom; proportions exaggerated for legibility). The field
// being edited or whose label was tapped is highlighted.
defineProps<{ kind: 'turning' | 'threading'; highlight: string | null }>()

// Threading sketch: the work's surface and thread depth, the face (Z0), the thread end (Length)
// and where the run-out starts, in viewBox units.
const surfaceY = 130
const rootY = 165
const faceX = 300
const endX = 110
const runOutX = 150
const pitchPx = 24

/** Outline of the threaded work: crests every pitch, roots at full depth, rising to the surface
 *  over the run-out (where the tool pulls out) so the groove ends at the thread end. */
function outline(): string {
  const points: [number, number][] = [
    [40, surfaceY],
    [endX, surfaceY],
  ]
  const crests: number[] = []
  for (let x = faceX; x > endX; x -= pitchPx) {
    crests.push(x)
  }
  crests.reverse()
  for (const crest of crests) {
    const root = crest - pitchPx / 2
    if (root > endX) {
      const share = root >= runOutX ? 1 : (root - endX) / (runOutX - endX)
      points.push([root, surfaceY + (rootY - surfaceY) * share])
    }
    points.push([crest, surfaceY])
  }
  points.push([faceX, 300], [40, 300])
  return points.map(([x, y]) => `${String(x)},${String(y)}`).join(' ')
}

const threadOutline = outline()
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
    <polygon points="40,70 150,70 150,145 314,167.8 330,184 330,300 40,300" class="part" />
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
      <text x="356" y="112" class="label" text-anchor="end">Cut depth</text>
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

    <!-- Chamfer: 45° on the finished edge at the face -->
    <g :class="{ on: highlight === 'chamfer' }">
      <line x1="314" y1="167.8" x2="330" y2="184" class="final" />
      <text x="322" y="214" class="label" text-anchor="end">Chamfer 45°</text>
    </g>

    <!-- Target: finished size at the face -->
    <g :class="{ on: highlight === 'target' }">
      <line x1="330" y1="170" x2="398" y2="170" class="ext" />
      <line x1="392" y1="300" x2="392" y2="172" class="dim" marker-start="url(#arrow)" marker-end="url(#arrow)" />
      <text x="400" y="240" class="label" text-anchor="middle" transform="rotate(-90 400 240)">Target ⌽</text>
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

    <!-- One pass of the tool, drawn in its own lane above the work (same Z positions). -->
    <text x="12" y="20" class="note">tool path, one pass</text>
    <g>
      <line x1="110" y1="36" x2="368" y2="36" class="rapid" />
      <line x1="368" y1="36" x2="368" y2="52" class="rapid" />
      <text x="240" y="30" class="note" text-anchor="middle">rapid back to the start</text>
    </g>
    <polygon points="368,57 361,44 375,44" class="tool" />
    <text x="414" y="52" class="note" text-anchor="end">start</text>
    <g :class="{ on: highlight === 'infeed_deg' }">
      <line x1="368" y1="59" x2="358" y2="80" class="infeed" marker-end="url(#arrow-t)" />
      <text x="414" y="98" class="label" text-anchor="end">Infeed ∠</text>
    </g>
    <line x1="356" y1="80" x2="152" y2="80" class="cut" marker-end="url(#arrow-t)" />
    <text x="254" y="74" class="note" text-anchor="middle">cut, synchronized to the spindle</text>
    <g :class="{ on: highlight === 'run_out' }">
      <line x1="150" y1="80" x2="112" y2="59" class="pullout" />
      <line x1="150" y1="84" x2="150" y2="130" class="ext" />
      <line x1="110" y1="62" x2="110" y2="130" class="ext" />
      <line x1="112" y1="100" x2="148" y2="100" class="dim" marker-start="url(#arrow-t)" marker-end="url(#arrow-t)" />
      <text x="46" y="104" class="label">Run-out</text>
    </g>
    <g :class="{ on: highlight === 'retract' }">
      <line x1="110" y1="57" x2="110" y2="40" class="dim" marker-end="url(#arrow-t)" />
      <text x="56" y="52" class="label">Retract</text>
    </g>

    <!-- The work: full-depth thread from the face, shallowing over the run-out to the end. -->
    <rect x="14" y="118" width="26" height="182" class="chuck" />
    <polygon :points="threadOutline" class="part" />
    <line x1="10" y1="300" x2="410" y2="300" class="centre" />
    <text x="44" y="292" class="note on-part">centreline</text>
    <line x1="300" y1="122" x2="300" y2="324" class="ext" />
    <text x="304" y="327" class="note">Z0 (face)</text>

    <g :class="{ on: highlight === 'pitch' }">
      <line x1="228" y1="130" x2="228" y2="116" class="ext" />
      <line x1="252" y1="130" x2="252" y2="116" class="ext" />
      <line x1="230" y1="119" x2="250" y2="119" class="dim" marker-start="url(#arrow-t)" marker-end="url(#arrow-t)" />
      <text x="240" y="112" class="label" text-anchor="middle">Pitch</text>
    </g>
    <g :class="{ on: highlight === 'depth' || highlight === 'allowance' }">
      <line x1="300" y1="165" x2="330" y2="165" class="ext" />
      <line x1="300" y1="130" x2="330" y2="130" class="ext" />
      <line x1="324" y1="132" x2="324" y2="163" class="dim" marker-start="url(#arrow-t)" marker-end="url(#arrow-t)" />
      <text x="330" y="152" class="label">Depth</text>
    </g>
    <g :class="{ on: highlight === 'length' }">
      <line x1="110" y1="130" x2="110" y2="324" class="ext" />
      <line x1="112" y1="318" x2="298" y2="318" class="dim" marker-start="url(#arrow-t)" marker-end="url(#arrow-t)" />
      <text x="205" y="313" class="label" text-anchor="middle">Length</text>
    </g>

    <!-- Taper: the thread is cut on a cone that is turned first. -->
    <g transform="translate(52 186)" :class="{ on: highlight === 'taper_deg' }">
      <text x="0" y="0" class="note on-part">Taper ∠: thread on a turned cone</text>
      <polygon points="0,14 104,30 104,62 0,62" class="cone" />
      <polyline points="8,15 14,23 20,17 26,25 32,19 38,27 44,21 50,29 56,23 62,31 68,25 74,33 80,27 86,35 92,29" class="cone-thread" />
      <line x1="104" y1="30" x2="44" y2="30" class="ext" />
      <path d="M 74 30 A 30 30 0 0 1 74.4 25.4" class="dim" />
      <text x="56" y="44" class="label small">∠</text>
    </g>

    <!-- Depth per pass: 1stCut, then x Factor each pass, never below Min cut; Spring at full depth. -->
    <g transform="translate(232 196)">
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

.cut {
  fill: none;
  stroke: var(--text);
  stroke-width: 1.6;
}

.cone {
  fill: var(--control-hover);
  stroke: var(--text-faint);
}

.cone-thread {
  fill: none;
  stroke: var(--text-dim);
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

.on .cone,
.on .cone-thread {
  stroke: var(--entry);
}

.on .bar {
  fill: var(--entry);
  stroke: var(--entry);
}
</style>
