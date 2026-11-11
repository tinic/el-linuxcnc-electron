// Display conversion. The machine side works in millimetres with X as a radius; the panel shows
// inches when the operator switches units and X as a diameter in diameter mode.

export const mmPerInch = 25.4

export function lengthToDisplay(mm: number, metric: boolean): number {
  return metric ? mm : mm / mmPerInch
}

export function lengthFromDisplay(value: number, metric: boolean): number {
  return metric ? value : value * mmPerInch
}

export function lengthDecimals(metric: boolean): number {
  return metric ? 3 : 4
}

export function formatLength(mm: number, metric: boolean): string {
  return lengthToDisplay(mm, metric).toFixed(lengthDecimals(metric))
}

/** X position as shown: diameter in diameter mode. */
export function xToDisplay(radiusMm: number, metric: boolean, diameterMode: boolean): number {
  return lengthToDisplay(diameterMode ? radiusMm * 2 : radiusMm, metric)
}

export function xFromDisplay(value: number, metric: boolean, diameterMode: boolean): number {
  const mm = lengthFromDisplay(value, metric)
  return diameterMode ? mm / 2 : mm
}

/** A pitch label like the preset buttons show: "1.5mm", or threads per inch in inch mode. */
export function pitchLabel(pitchMm: number, metric: boolean): string {
  if (metric) {
    return `${String(Number(pitchMm.toFixed(4)))}mm`
  }
  const tpi = mmPerInch / pitchMm
  const rounded = Math.round(tpi)
  return `${Math.abs(tpi - rounded) < 0.1 ? String(rounded) : tpi.toFixed(1)} TPI`
}
