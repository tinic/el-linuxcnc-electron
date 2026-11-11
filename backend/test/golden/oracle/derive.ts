// Runs the REAL useCannedCycles.ts parameter derivation for every preset.
// Usage: bundled by esbuild, then: node derive.mjs <threadpresets.json> <turningpresets.json> > derived.json
import { readFileSync } from 'node:fs'
import { useCannedCycles } from '../../../../elle-app/elle-frontend/src/composables/useCannedCycles'

const threadPresets = JSON.parse(readFileSync(process.argv[2], 'utf8'))
const turningPresets = JSON.parse(readFileSync(process.argv[3], 'utf8'))
const c = useCannedCycles()
const r = (v: number) => c.roundParameterValue(v)
const positions = [ { x: 0, z: 0, a: 0 }, { x: 5.25, z: -1.5, a: 0.125 } ]
const out: unknown[] = []

for (const [group, list] of Object.entries(threadPresets) as [string, any[]][]) {
  for (const p of list) {
    for (const pos of positions) {
      c.threadPitch.value = r(p.Pitch); c.threadXDepth.value = r(p.XDepth); c.threadZDepth.value = r(p.ZDepth)
      c.threadAngle.value = r(p.Angle || 0); c.threadZEnd.value = r(p.ZEnd); c.threadXPullout.value = r(p.XPullout)
      c.threadZPullout.value = r(p.ZPullout); c.threadFirstCut.value = r(p.FirstCut); c.threadCutMult.value = r(p.CutMult)
      c.threadMinCut.value = r(p.MinCut); c.threadSpringCuts.value = r(p.SpringCuts)
      out.push({ kind: 'threading', name: `${group}/${p.name}`, position: pos,
        user: { pitch: c.threadPitch.value, x_depth: c.threadXDepth.value, z_depth: c.threadZDepth.value, angle_deg: c.threadAngle.value,
                z_end: c.threadZEnd.value, x_pullout: c.threadXPullout.value, z_pullout: c.threadZPullout.value, first_cut: c.threadFirstCut.value,
                cut_mult: c.threadCutMult.value, min_cut: c.threadMinCut.value, spring_cuts: c.threadSpringCuts.value },
        params: c.generateThreadingParams(pos.x, pos.z, pos.a) })
    }
  }
}
for (const [group, list] of Object.entries(turningPresets) as [string, any[]][]) {
  for (const p of list) {
    for (const stockExtra of [1.3, 0.05]) {
      const pos = { x: p.target + stockExtra, z: 0, a: 0 }
      c.turningTarget.value = r(p.target); c.turningZEnd.value = r(p.zEnd); c.turningFeedRate.value = r(p.feedRate)
      c.turningStepDown.value = r(p.stepDown); c.turningFinalStepDown.value = r(p.finalStepDown)
      c.turningTaperAngle.value = r(p.taperAngle); c.turningSpringPasses.value = r(p.springPasses || 0)
      out.push({ kind: 'turning', name: `${group}/${p.name}/stock+${stockExtra}`, position: pos,
        user: { target: c.turningTarget.value, z_end: c.turningZEnd.value, feed_rate: c.turningFeedRate.value, step_down: c.turningStepDown.value,
                final_step_down: c.turningFinalStepDown.value, taper_angle_deg: c.turningTaperAngle.value, spring_passes: c.turningSpringPasses.value },
        params: c.generateTurningParams(pos.x, pos.z, pos.a) })
    }
  }
}
process.stdout.write(JSON.stringify(out, null, 1))
