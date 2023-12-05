# Vulkan engine

## TODO
- Fix vulkan cleanup


## Goal
- Skeletal animation
    - Export animation from blender
    - Export model rig from blender
        - Skeleton info
        - Each vertex has iv3 bone count and v3 bone weight
    - Import model
    - Create uniform buffer for bone transforms
    - Lerp between keyframes and write transform to uniform buffer
    - Calculate resulting position in vertex shader
- Good looking shadows (directional)
- Basic water rendering
- Object loading
- SSAO
- Postprocessing: Tonemapping and Dithering? Depth of Field? Motion blur? Bloom?
- Antialiasing
