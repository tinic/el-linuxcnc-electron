<template>
  <div class="operation-preview">
    <div class="preview-header">
      <h4 class="dro-font-mode">{{ operation.name }} Preview</h4>
    </div>

    <div class="preview-content">
      <!-- Operation Details -->
      <div class="operation-details">
        <h4 class="dro-font-mode text-sm mb-2">Operation Details:</h4>
        <div class="details-grid">
          <template v-for="(value, key) in operation.parameters" :key="key">
            <div class="detail-row">
              <span class="detail-label">{{ formatParameterName(key) }}:</span>
              <span class="detail-value">{{ formattedParameters[key] }}</span>
            </div>
          </template>
        </div>
      </div>

      <!-- 3D Backplot -->
      <div class="backplot-container">
        <div ref="backplotContainer" class="backplot-viewer">
          <Renderer ref="rendererRef" antialias>
            <Camera :position="cameraPosition" :look-at="cameraTarget" />
            <Scene />
          </Renderer>
        </div>
      </div>
    </div>

    <div class="preview-actions">
      <button
        class="action-button cancel-button dro-font-mode"
        style="width: 8em;"
        @click="$emit('cancel')"
      >
        ❌ Cancel
      </button>
      <button
        class="action-button continue-button dro-font-mode"
        style="width: 8em;"
        @click="$emit('continue')"
      >
        ✅ Start
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, watch, computed } from 'vue'
import { Camera, Renderer, RendererPublicInterface, Scene } from 'troisjs'
import * as THREE from 'three'
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls'
import { useSettings } from '../composables/useSettings'

interface OperationData {
  name: string;
  type: string;
  parameters: Record<string, any>;
  gcode: string[];
  backplotData?: any;
}

const props = defineProps<{
  operation: OperationData;
}>()

// eslint-disable-next-line no-unused-vars
const emit = defineEmits<{
  cancel: [];
  continue: [];
}>()

const rendererRef = ref<RendererPublicInterface>()
const backplotContainer = ref<HTMLElement>()
const { diameterMode } = useSettings()

// Computed property for formatted parameters that reacts to diameter mode changes
const formattedParameters = computed(() => {
  const formatted: Record<string, string> = {}
  
  for (const [key, value] of Object.entries(props.operation.parameters)) {
    formatted[key] = formatParameterValue(key, value)
  }
  
  return formatted
})

// Reactive camera positioning
const cameraPosition = ref({ x: 0, y: 1.25, z: 0 })
const cameraTarget = ref({ x: 0, y: 0, z: 0 })

const formatParameterName = (key: string): string => {
  const paramConfig = getParameterConfig(key)
  return paramConfig.displayName
}

const getParameterConfig = (key: string) => {
  const parameterMap: Record<string, { displayName: string, doubleInDiameterMode: boolean, isInteger: boolean }> = {
    'Pitch': { displayName: 'Pitch', doubleInDiameterMode: false, isInteger: false },
    'XDepth': { displayName: 'X Depth', doubleInDiameterMode: true, isInteger: false },
    'ZDepth': { displayName: 'Z Depth', doubleInDiameterMode: false, isInteger: false },
    'ZEnd': { displayName: 'Z End', doubleInDiameterMode: false, isInteger: false },
    'XPullout': { displayName: 'X Pullout', doubleInDiameterMode: true, isInteger: false },
    'ZPullout': { displayName: 'Z Pullout', doubleInDiameterMode: false, isInteger: false },
    'FirstCut': { displayName: 'First Cut', doubleInDiameterMode: true, isInteger: false },
    'CutMult': { displayName: 'Cut Multiplier', doubleInDiameterMode: false, isInteger: false },
    'MinCut': { displayName: 'Min Cut', doubleInDiameterMode: true, isInteger: false },
    'SpringCuts': { displayName: 'Spring Cuts', doubleInDiameterMode: false, isInteger: true },
    'Target': { displayName: 'Target', doubleInDiameterMode: true, isInteger: false },
    'Stock': { displayName: 'Stock', doubleInDiameterMode: true, isInteger: false },
    'FeedRate': { displayName: 'Feed Rate', doubleInDiameterMode: false, isInteger: false },
    'StepDown': { displayName: 'Step Down', doubleInDiameterMode: true, isInteger: false },
    'FinalStepDown': { displayName: 'Final Step', doubleInDiameterMode: true, isInteger: false },
    'SpringPasses': { displayName: 'Spring Passes', doubleInDiameterMode: false, isInteger: true },
    'TaperAngle': { displayName: 'Taper Angle', doubleInDiameterMode: false, isInteger: false },
    'XStart': { displayName: 'X Start', doubleInDiameterMode: true, isInteger: false },
    'XEnd': { displayName: 'X End', doubleInDiameterMode: true, isInteger: false }
  }
  
  return parameterMap[key] || { displayName: key, doubleInDiameterMode: false, isInteger: false }
}

const formatParameterValue = (key: string, value: any): string => {
  if (value === null || value === undefined) {return 'Not set'}
  if (typeof value === 'number') {
    const paramConfig = getParameterConfig(key)
    let displayValue = value
    
    // Apply diameter mode doubling if applicable
    if (paramConfig.doubleInDiameterMode && diameterMode.value) {
      displayValue = value * 2
    }
    
    // Format based on whether it's an integer parameter
    if (paramConfig.isInteger) {
      return Math.round(displayValue).toString()
    } else {
      return displayValue.toFixed(3)
    }
  }
  return String(value)
}

const setupBackplot = () => {
  if (!rendererRef.value || !props.operation.backplotData) {
    return
  }

  const renderer = rendererRef.value

  // Force renderer to resize to container size
  const containerElement = backplotContainer.value
  if (containerElement && renderer.renderer) {
    renderer.renderer.setSize(containerElement.clientWidth, containerElement.clientHeight)
  }

  // Clear existing scene
  if (renderer.scene) {
    renderer.scene.clear()
  }

  // Create simple backplot using basic Three.js lines
  const backplotData = props.operation.backplotData
  const extents = backplotData.extents

  if (extents && extents.length >= 6) {
    // const [xmin, ymin, zmin, xmax, ymax, zmax] = extents

    /*
    // Create bounding box
    const boxGeometry = new THREE.BoxGeometry(
      Math.abs(xmax - xmin),
      Math.abs(ymax - ymin),
      Math.abs(zmax - zmin)
    );
    const boxEdges = new THREE.EdgesGeometry(boxGeometry);
    const boxMaterial = new THREE.LineBasicMaterial({ color: 0x888888 });
    const box = new THREE.LineSegments(boxEdges, boxMaterial);
    box.position.set(
      (xmin + xmax) / 2,
      (ymin + ymax) / 2,
      (zmin + zmax) / 2
    );
    renderer.scene?.add(box);
  */
    // Create cross marker at starting position (without lead-in)
    const createStartingPositionCross = () => {
      const params = props.operation.parameters

      // Get the actual starting position from operation parameters
      let startX = 0
      let startZ = 0

      if (params.Target) {
        // For turning operations, start at current position (Stock)
        startX = params.Stock ? parseFloat(params.Stock) : 0
        startZ = 0 // Turning always starts at Z=0
      } else if (params.XStart && params.ZStart) {
        // For threading operations
        startX = parseFloat(params.XStart)
        startZ = parseFloat(params.ZStart)
      }

      // Transform to normalized coordinates (same transform as backend)
      if (backplotData.transform) {
        const transform = backplotData.transform
        const [centerX, centerY, centerZ] = transform.center
        const scaleFactor = transform.scale_factor

        // Apply same coordinate transformation as backend: X,Y,Z -> Z,Y,X
        const normX = (startZ - centerZ) * scaleFactor // startZ becomes normX
        const normY = (0 - centerY) * scaleFactor      // Y is always 0 for lathe
        const normZ = (startX - centerX) * scaleFactor // startX becomes normZ

        // Create cross geometry
        const crossSize = 0.1
        const crossGeometry = new THREE.BufferGeometry()
        const crossVertices = new Float32Array([
          // Horizontal line
          normX - crossSize, normY, normZ,
          normX + crossSize, normY, normZ,
          // Vertical line
          normX, normY, normZ - crossSize,
          normX, normY, normZ + crossSize
        ])
        crossGeometry.setAttribute('position', new THREE.BufferAttribute(crossVertices, 3))

        const crossMaterial = new THREE.LineBasicMaterial({
          color: 0xff0000, // Bright red
          linewidth: 3,
          depthTest: false,
          depthWrite: false
        })

        const cross = new THREE.LineSegments(crossGeometry, crossMaterial)
        cross.renderOrder = 3000 // Render on top of everything
        renderer.scene?.add(cross)
      }
    }

    createStartingPositionCross()

    // Create 2D stock visualization
    const createStockVisualization = () => {
      const params = props.operation.parameters

      if (params.Stock && params.ZEnd) {
        // For turning operations, show 2D stock profile
        const stockRadius = params.Stock ? parseFloat(params.Stock) : 0
        const zEnd = parseFloat(params.ZEnd)
        const zStart = 0

        // Transform to normalized coordinates
        if (backplotData.transform) {
          const transform = backplotData.transform
          const [centerX, , centerZ] = transform.center
          const scaleFactor = transform.scale_factor

          // Create solid 2D rectangular profile representing the stock
          const stockGeometry = new THREE.BufferGeometry()
          const vertices = new Float32Array([
            // Triangle 1
            (zStart - centerZ) * scaleFactor, 0, (0 - centerX) * scaleFactor,
            (zEnd - centerZ) * scaleFactor, 0, (0 - centerX) * scaleFactor,
            (zStart - centerZ) * scaleFactor, 0, (stockRadius - centerX) * scaleFactor,
            // Triangle 2
            (zEnd - centerZ) * scaleFactor, 0, (0 - centerX) * scaleFactor,
            (zEnd - centerZ) * scaleFactor, 0, (stockRadius - centerX) * scaleFactor,
            (zStart - centerZ) * scaleFactor, 0, (stockRadius - centerX) * scaleFactor
          ])

          stockGeometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3))

          const stockMaterial = new THREE.MeshBasicMaterial({
            color: 0x0088ff, // Blue
            transparent: true,
            opacity: 0.3,
            side: THREE.DoubleSide
          })

          const stockOutline = new THREE.Mesh(stockGeometry, stockMaterial)
          stockOutline.renderOrder = 1 // Render behind toolpath
          renderer.scene?.add(stockOutline)
        }
      }
    }

    createStockVisualization()

    // Create unique materials for each line to avoid sharing issues
    const lines: { line: THREE.Line, entryType: string, materials: { future: THREE.LineBasicMaterial, active: THREE.LineBasicMaterial, completed: THREE.LineBasicMaterial } }[] = []
    let lineCount = 0

    for (const entry of backplotData.backplot) {
      const entryType = entry.type
      if (entryType === 'feed' || entryType === 'arcfeed' || entryType === 'trav') {
        const moves = entry[entryType]
        for (const move of moves) {
          if (move.coords && move.coords.length >= 6) {
            const points = [
              new THREE.Vector3(move.coords[0], move.coords[1], move.coords[2]),
              new THREE.Vector3(move.coords[3], move.coords[4], move.coords[5])
            ]
            const lineGeometry = new THREE.BufferGeometry().setFromPoints(points)

            // Create unique materials for this line with proper depth handling
            const materials = {
              future: new THREE.LineBasicMaterial({
                color: (entryType === 'trav') ? 0x804080 : 0x404040,
                linewidth: 1,
                depthTest: false,
                depthWrite: false
              }),
              active: new THREE.LineBasicMaterial({
                color: 0xffffff,  // White highlight for active line
                linewidth: 4,
                depthTest: false,
                depthWrite: false
              }),
              completed: new THREE.LineBasicMaterial({
                color: (entryType === 'trav') ? 0xff00ff : 0x00ff00,
                linewidth: 2,
                depthTest: false,
                depthWrite: false
              })
            }

            // Start with future material
            const line = new THREE.Line(lineGeometry, materials.future)

            renderer.scene?.add(line)
            lines.push({ line, entryType, materials })
            lineCount++
          }
        }
      }
    }

    // Add orbit controls for interaction
    if (renderer.camera && renderer.renderer?.domElement) {
      const controls = new OrbitControls(renderer.camera, renderer.renderer.domElement)
      controls.enableDamping = true
      controls.dampingFactor = 0.05
      controls.enableZoom = true
      controls.enableRotate = true
      controls.enablePan = true
      controls.maxDistance = 10.0
      controls.minDistance = 0.1

      controls.target.set(0, 0, 0)

      // Force camera position through orbit controls
      if (renderer.camera) {
        renderer.camera.position.set(cameraPosition.value.x, cameraPosition.value.y, cameraPosition.value.z)
        renderer.camera.lookAt(cameraTarget.value.x, cameraTarget.value.y, cameraTarget.value.z)
      }

      controls.update();

      // Store controls reference for cleanup
      (renderer as any).orbitControls = controls
    }

    // Create lathe tool triangle indicator as solid equilateral triangle - stays in X-Z plane
    const createToolTriangle = () => {
      const size = 0.06
      const height = size * Math.sqrt(3) / 2 // Height of equilateral triangle

      const toolGeometry = new THREE.BufferGeometry()
      const vertices = new Float32Array([
        0, 0, 0,                    // tip (cutting point)
        -height, 0, size / 2,         // back left
        -height, 0, -size / 2         // back right
      ])
      toolGeometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3))
      toolGeometry.setIndex([0, 1, 2]) // Triangle face

      const toolMaterial = new THREE.MeshBasicMaterial({
        color: 0xff0000, // Bright red for visibility
        side: THREE.DoubleSide
      })

      const toolTriangle = new THREE.Mesh(toolGeometry, toolMaterial)
      renderer.scene?.add(toolTriangle)

      return toolTriangle
    }

    const toolTriangle = createToolTriangle()

    // Animation loop for toolpath progress with smooth interpolation
    let animationTime = 0
    const pauseDuration = 60 // 1 second pause at 60fps
    const animationDuration = 1200 // 20 seconds for the actual animation
    const totalDuration = pauseDuration + animationDuration + pauseDuration // pause + animation + pause

    renderer.onBeforeRender(() => {
      animationTime++

      let currentLine = 0
      let lineProgress = 0

      if (animationTime < pauseDuration) {
        // Initial pause - stay at beginning
        currentLine = 0
        lineProgress = 0
      } else if (animationTime < pauseDuration + animationDuration) {
        // Active animation phase
        const animProgress = (animationTime - pauseDuration) / animationDuration
        currentLine = animProgress * lineCount
        lineProgress = currentLine - Math.floor(currentLine)
      } else if (animationTime < totalDuration) {
        // Final pause - stay at end
        currentLine = lineCount - 1
        lineProgress = 1
      } else {
        // Reset animation
        animationTime = 0
        currentLine = 0
        lineProgress = 0
      }

      // Smooth line material updates
      const currentLineIndex = Math.floor(currentLine)

      lines.forEach((lineData, index) => {
        const { line, materials } = lineData

        if (index < currentLineIndex) {
          // Completed lines - render on top of future lines
          line.material = materials.completed
          line.renderOrder = 1000 + index
        } else if (index === currentLineIndex) {
          // Active line - render on top of everything
          line.material = materials.active
          line.renderOrder = 2000 + index
        } else if (index === currentLineIndex + 1 && lineProgress > 0.7) {
          // Next line starts to highlight when current is 70% done
          line.material = materials.active
          line.renderOrder = 2000 + index
        } else {
          // Future lines - render under everything else
          line.material = materials.future
          line.renderOrder = index // Low render order
        }
      })

      // Update tool triangle position to current cutting point
      if (currentLineIndex < lines.length) {
        const currentLineData = lines[currentLineIndex]
        if (currentLineData) {
          const line = currentLineData.line
          const geometry = line.geometry as THREE.BufferGeometry
          const positions = geometry.attributes.position.array as Float32Array

          // Interpolate position along current line
          const startX = positions[0]
          const startY = positions[1]
          const startZ = positions[2]
          const endX = positions[3]
          const endY = positions[4]
          const endZ = positions[5]

          const toolX = startX + (endX - startX) * lineProgress
          const toolY = startY + (endY - startY) * lineProgress
          const toolZ = startZ + (endZ - startZ) * lineProgress

          toolTriangle.position.set(toolX, toolY, toolZ)

          // Rotate triangle to point in direction of cut
          const direction = new THREE.Vector3(endX - startX, endY - startY, endZ - startZ).normalize()
          const angle = Math.atan2(direction.z, direction.x)
          toolTriangle.rotation.y = -angle
        }
      }

      // Update orbit controls
      if ((renderer as any).orbitControls) {
        (renderer as any).orbitControls.update()
      }
    })
  }
}

// Watch for backplot data changes
watch(() => props.operation.backplotData, () => {
  if (props.operation.backplotData) {
    setTimeout(setupBackplot, 100)
  }
}, { immediate: true })

onMounted(() => {
  if (props.operation.backplotData) {
    setTimeout(setupBackplot, 200)
  }
})
</script>

<style scoped>
.operation-preview {
  display: flex;
  flex-direction: column;
  height: 90vh;
  max-width: 95vw;
  background: #222;
  color: #fff;
  border-radius: 8px;
  overflow: hidden;
}

.preview-header {
  padding: 0.5rem;
  background: #333;
  border-bottom: 1px solid #555;
  text-align: center;
}

.preview-header h4 {
  margin: 0;
  font-size: 1.1em;
}

.preview-content {
  display: flex;
  flex: 1;
  min-height: 500px;
  overflow: hidden;
}

.operation-details {
  width: 200px;
  padding: 1rem;
  background: #2a2a2a;
  border-right: 1px solid #555;
  overflow-y: auto;
  flex-shrink: 0;
}

.operation-details h4 {
  margin: 0 0 1rem 0;
  color: #fff;
  border-bottom: 1px solid #444;
  padding-bottom: 0.5rem;
}

.details-grid {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.detail-row {
  display: flex;
  justify-content: space-between;
  padding: 0.25rem 0;
  border-bottom: 1px solid #333;
}

.detail-label {
  font-weight: bold;
  color: #ccc;
}

.detail-value {
  color: #fff;
  font-family: "iosevka", monospace;
}

.backplot-container {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
  height: 100%;
}

.backplot-viewer {
  background: #1a1a1a;
  flex: 1;
  position: relative;
  overflow: hidden;
  width: 100%;
  height: 100%;
}

.preview-actions {
  padding: 1rem;
  background: #333;
  border-top: 1px solid #555;
  display: flex;
  justify-content: center;
  gap: 1rem;
  flex-shrink: 0;
}

.action-button {
  padding: 0.5rem 1rem;
  border: none;
  border-radius: 4px;
  font-size: 1.1em;
  cursor: pointer;
  transition: background-color 0.2s;
}

.cancel-button {
  background: #dc2626;
  color: white;
}

.cancel-button:hover {
  background: #b91c1c;
}

.continue-button {
  background: #16a34a;
  color: white;
}

.continue-button:hover {
  background: #15803d;
}

.dro-font-mode {
  font-family: "iosevka";
  font-weight: bold;
}
</style>
