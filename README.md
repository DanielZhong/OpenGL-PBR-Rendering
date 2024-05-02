# OpenGL-Rendering-Projects

* Ruijun(Daniel) Zhong
    * [LinkedIn](https://www.linkedin.com/in/daniel-z-73158b152/)    
    * [Personal Website](https://www.danielzhongportfolio.com/)
* Tested on: 
  * Core(TM) i7-12700K 3.61 GHz 32.0 GB, NVIDIA GeForce RTX 3070 Ti (personal computer)   
    * QT 6

# Real Time Rendering
## PBR Cook-Torrance BRDF(Environment Map)
Following project  outlines the implementation of **Physically Based Shading (PBS)** in OpenGL, focusing on realistic material rendering. The approach is inspired by real-world physics principles and advanced computational techniques to simulate light interaction with surfaces. This method enhances visual realism, particularly for metallic and rough surfaces.  

The Cook-Torrance BRDF is a model used in computer graphics to simulate light interaction with rough surfaces, ideal for rendering realistic materials like metals and plastics. It combines Fresnel reflectance, geometric attenuation, and microfacet distribution to account for light reflection and scattering. The formula:  

fr(ωi, ωo) = [F(ωi, h) * G(ωi, ωo, h) * D(h)] / [4 * (ωi · n) * (ωo · n)]
fr(ωi, ωo): BRDF function, ratio of reflected radiance to incident irradiance.  

* ωi: Incident light direction.
* ωo: Reflected light direction.
* h: Half-vector between ωi and ωo.
* n: Surface normal.
* F: Fresnel term for light reflection at angles.
* G: Geometric attenuation for shadowing/masking.
* D: Microfacet distribution for surface microstructure. 

This implementation is base on the paper [Karis, B. (2013). Real Shading in Unreal Engine 4](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)

Insights from the paper:  
**Assuming wo as Surface Normal**: A simplification made in this approach is to treat the view direction vector (wo) as equivalent to the surface normal. This assumption helps reduce complexity and computational overhead during certain calculations, particularly in the glossy reflection precomputation.  
**Generating wi Samples with Hammersley Sequence**: During the precomputation of the glossy component, the Hammersley sequence—a low-discrepancy quasi-random sequence—is used to generate sample vectors (wi). These samples are distributed around the halfway vector (wh) and are crucial for importance sampling, ensuring that the samples are more focused around the primary reflection directions based on surface roughness.

Features:
* **Screen Space Reflection (SSR)**: Screen Space Reflection (SSR) is a rendering technique that enhances visual realism by enabling objects within a scene to reflect both themselves and their surroundings. The process starts with a G-buffer that captures essential geometric data, including depth and normals. Using this data, SSR employs ray marching to trace rays within the screen space, identifying reflection points on surfaces. This allows objects to reflect their surroundings and themselves when visible in the screen space. After ray tracing, Gaussian blur is applied to soften the reflections, ensuring they blend naturally with the scene, thus significantly enhancing visual immersion.
* **Subsurface Scattering  (SSS)**: Subsurface Scattering is a technique used in rendering to simulate the light that penetrates a translucent material, scatters internally, and exits from a different location. This effect is crucial for rendering materials like skin, wax, marble, or leaves, where light does not just bounce off the surface but travels through the material, creating a softer, diffused look. SSS enhances the realism of digital objects by mimicking the way light interacts with the internal structure of these semi-transparent materials, lending them a more lifelike appearance.
* **Mipmapping for Roughness**: Utilizes mipmaps to dynamically adjust surface roughness, enhancing detail and minimizing artifacts by preventing excessive sampling of overly bright light sources.  
* **Precomputation Techniques**: Employs precomputation for both diffuse and glossy components, optimizing runtime performance.  
* **Gamma Correction**: Ensures accurate color representation by applying gamma correction to the final rendered image.  
* **Displacement and Normal Mapping**: Incorporates displacement mapping for geometric detail and normal mapping for surface complexity.  
* **Optimizations**: employs mipmapping, binaray search Ray March and Hammersley sequences for efficiency.

|<img src="Result/r2.jpg" width="100%">|<img src="Result/r3.jpg" width="100%">|<img src="Result/r4.jpg" width="100%">|
|:-:|:-:|:-:|
|0% metallic, 0% rough, RBG = 0 0 0|100% metallic, 0% rough, RGB = 0 0 0|100% metallic, 25% rough, RGB = 1 1 1|
|<img src="Result/r1.jpg" width="100%">|<img src="Result/r5.jpg" width="100%">|<img src="Result/r6.jpg" width="100%">|
|Custom Texture|Displacement Map|Normal Map|
|<img src="Result/SSR.jpg" width="100%">|<img src="Result/SSR2.jpg" width="100%">|<img src="Result/SSR3.jpg" width="100%">|
|SSR|SSR|SSR|
|<img src="result/subsurface_scattering_original.jpg" width="100%">|<img src="result/subsurface_scattering1.jpg" width="100%">|<img src="result/subsurface_scattering.jpg" width="100%">|
|Original|SSS Color|SSS Result|

### **Signed Distance Function (SDF)**: A mathematical representation used to describe objects in a scene by calculating the shortest distance from any point in space to the surface of an object. Combine effect to PBR Cook Torance Effect:
## [DEMO VIDEO: SDF ](https://drive.google.com/file/d/1W4WBu46-Y60jNKJAC1WHqdd-lvt4BKIu/view?usp=sharing)
<img src="result/Infinity.jpg">

## Mini Minecraft Game Engine (In progress adding PBR effects)
 ### Key features:
 * Shadow Mapping
 * Deferred Rendering Pipline
 * Player Movement & Intersections
 * Multithread & Infinity Procedural Terrian/Skybox Generations
 * Distance Fog (blend color with procedural skybox)
 
<details>
  <summary> Click for details </summary>
Team Member: Ruijun Zhong, Yuetian Zhao, Keqi Wu

# Milestone1
<details>
  <summary> Specifications </summary>
  
## Game Engine Tick Function and Player Physics (Ruijun Zhong)
**Player Movement Control**
  * **F key**: Enable or disable flight mode
  * **Mouse Movement**: Control Camera Orientation using polar coordinates.
  * **WASD Keys**: directional player movement relative to the camera's orientation, enhancing navigation within the voxel-based environment.
  * **Space Key**: Activates the jump functionality, allowing the player to overcome obstacles and explore vertical terrain features.
  * **Q&E Keys**: Accelerate postively or negatively along up vector during flight mode
  * **Shift  Key**: Engages a speed multiplier, providing the capability for accelerated movement across expansive terrains.  

**Physics**
  * **Gravity Simulation**: Applies a constant downward force on the player, necessitating strategic navigation and engagement with various terrain elevations.
  * **Friction Implementation**: Decelerates player movement over time, preventing unrealistic perpetual motion and enhancing the realism of player-terrain interactions.
  * **External Forces**:  Considers additional environmental forces, enabling dynamic player responses to various game world stimuli.

**Collision Detection**
  * **Grid Marching Technique**: Utilizes ray casting for precise collision prediction and prevention, ensuring terrain solidity is respected.
  * **Terrain Interaction:**: Modifies player movement based on detected voxel collisions, maintaining consistent and realistic interactions within the game environment.
  * **Ray Casting VS Object-Object Intersection Preference**: Selected over AABB intersection to improve collision detection reliability, crucial for avoiding missed collisions at lower frame rates and preventing "tunneling" through terrain.

  **Block Interaction**
  * **Left Mouse Click**: Removes a block from the terrain. This action utilizes a precise ray casting technique to determine the specific block the player is targeting, allowing for accurate and intuitive terrain modification.
  * **Right Mouse Click**: Places a new block adjacent to the targeted terrain block. The placement algorithm ensures the new block is positioned in direct relation to the block face the player is looking at, providing a seamless and intuitive building experience.
    
## Procedural Terrain (Yuetian Zhao)
**Grassland** : The grasslands terrain is generated using two-dimensional fractal Brownian Motion (fBM) driven by a Perlin noise function.

**mountains** : The mountains terrain is friven by fractal perlin noise, in order to make mountains sharper we use pow function and smoothstep for transition. 

**ProcTerrainGen** : The class stores all the noise function used in milestone1 and for future use.

## Efficient Terrain Rendering and Chunking (Keqi Wu)
**Chunk Inherited from drawable**

The primary goal is to accumulate VBO (Vertex Buffer Object) data for a chunk and store this data in memory to facilitate rendering. In the process of gathering VBO data, only the faces of opaque blocks that are adjacent to empty spaces (air) are considered for rendering. This entails appending the vertices, normals, colors, and UV coordinates of these faces to the VBO data. For blocks situated at the boundaries of a chunk, adjacent chunks are consulted to determine the status of blocks neighboring those at the edge.

**Interleaved VBO Data**

Given that the setup includes just a single buffer array besides the index buffer array, it's necessary only to use generateBuffer() along with POSITION, NORMAL, COLOR. Following this, the buffer data should be linked with POSITION, NORMAL, COLOR. Within shaderprogram.cpp, the addition of a drawInterleaved(Drawable &d) function facilitates the drawing of the buffer. This function details the starting points for each type of data—position, normal, color, UV coordinates—and outlines the stride required for accurately accessing each piece of information.

**Terrain expansion**

During each update cycle, the program verifies if the 81 chunks around the player, forming a 9 x 9 chunk area, have been initialized and whether their VBO data has been generated. If any chunks have not been created or their VBO data is missing, the program proceeds to instantiate these chunks and generate the necessary VBO data. There's a specific member variable, named m_ChunkVBOs, responsible for holding the VBOs of all chunks that are currently loaded. Within the draw function, there's a loop that traverses this chunk, rendering each chunk for which VBO data exists.
</details>

# Milestone2
<details>
  <summary> Specifications </summary>

## Cave Systems & Multithread(currently got issue here) (Keqi Wu)
* **Caves Generation**: 3d Perlin Noise was used to generate the cave systems, which are uniformly distributed beneath the entire surface terrain. If the noise value returned by getCaveHeight(x,y,z) is less than zero, we place STONE blocks; otherwise, we place LAVA or EMPTY blocks based on height.
*  **Collision Detection**: To prevent collisions with transparent objects, we do not set the velocity to zero if the hit block is transparent (WATER, LAVA).
*  **Post Processing**: Added color offset when under water and lava.
*  **multithread**: Also tried to implement another version of Multithread, containing some thread designing issues that cause crashes randomly. The terrain can be generated on my machine but not on team members machine (on MS1). When migrate to MS2, the program crashes.

## Texturing and Texture Animation (Ruijun Zhong)
**Player Movement Control**
  * **Sample From Texture Atlas**: Sample the texture from a texture atlas to consolidate multiple textures into a single program, reducing drawcalls and optimizing performance in real-time rendering.
  * **Texture Animation**: Animate the texture to enhance realism
  * **Transparent Blend Rendering**: Employ transparent blending techniques by rendering opaque objects first and transparent objects second, using separate Vertex Buffer Objects (VBOs) to avoid issues caused by the depth buffer. This approach ensures that if the transparent rendering is incorrect, it won't obstruct the visibility of other objects.

## Multithread & lava/water swim (Yuetian Zhao) 
**multithread**
  * **BlockTypeWorker**: use noise to generate terrain information
  * **VBOWorker**: used to generate data for our VBO and then pass to the shader.
  * **Swim**: swim in lava and water : change the velocity, so player can swim in the lava and water
</details>

# Milestone3
<details>
  <summary> Specifications </summary>

## Normal Mapping & Shdaow Map & Water Wave (Lighting) & Distance Fog & Defered Rendering Pipline(Ruijun Zhong)

* **Normal Map**: Apply Normal to make Minecraft Cube realistic

* **Shdaow Map**: For the Shadow Map implementation, Percentage-Closer Filtering (PCF) is utilized to enhance the visual quality of shadows. This technique softens the edges of shadows, making them appear more natural and less pixelated. The depth of objects from the light's perspective is recorded using a light view depth map, which is crucial for determining whether a pixel is in shadow or lighted. This method helps to accurately simulate the effect of shadows cast by light sources in 3D environments.
  
* **Water Wave (Lighting)**: The water wave simulation incorporates advanced lighting models to achieve realistic effects. Using the Blinn-Phong lighting model, specular highlights are rendered on the water surface based on the viewer and light direction, enhancing the visual perception of water surface undulations. Additionally, the Fresnel effect is employed to adjust the reflectivity of the water surface depending on the viewing angle; the water becomes more transparent as the viewing angle approaches grazing angles. This dynamic interaction of light with the water surface brings a lifelike quality to the scene.

* **Distance Fog**: The implementation of Distance Fog involves a technique that simulates atmospheric effects by gradually increasing the opacity of fog with distance from the camera. The color and density of the fog are dynamically adjusted based on the depth of the scene and a procedural skybox color, which allows for a seamless blend of the fog with the background sky, creating a depth cue and enhancing the perception of distance in the virtual environment.

* **Deferred Rendering Pipline**: The Deferred Rendering Pipeline is a powerful rendering technique used to handle multiple light sources efficiently in complex scenes. In this method, the rendering process is split into two main phases: the geometry pass and the lighting pass. During the geometry pass, data about scene geometry, such as positions, normals, and material properties, are captured in textures (G-buffers) without any lighting calculations. In the subsequent lighting pass, these textures are used to perform lighting calculations for each pixel independently, which allows for handling numerous dynamic lights and complex material interactions more effectively. This technique is especially beneficial for scenes with high geometric complexity and diverse lighting conditions.

## Day and night cycle & Post-process Camera Overlay & Water Wave (motion part) (Keqi Wu)

* **Day and night cycle**: Sky GLSL fragment shader crafts a dynamic sky environment by altering light and color based on the sun's continually changing position. Utilizing an inverse view projection matrix, it transforms screen coordinates to world coordinates, establishing the foundation for simulating atmospheric effects. The shader calculates ray directions from the camera, essential for rendering the light scattering across the sky. Worley noise generates animated, realistic cloud textures, contributing depth and movement to the sky. Sphere-to-UV mapping is employed to apply these textures onto a simulated spherical dome, enhancing the visual impression of a curved atmosphere. Color transitions are meticulously handled, shifting between distinct palettes for noon, sunset, and dusk based on the sun's elevation and angle relative to the observer. This blending is tuned to reflect the sun's position, with special effects like a glowing sun that dynamically changes in appearance and intensity. The result is a visually compelling sky simulation that enhances the realism and depth of 3D scenes, making the shader integral to immersive outdoor environments.
  
* **Water Wave (motion part)**: The vertex shader simulates water wave motion on geometry marked as "animated" by utilizing vertex attributes. This effect is achieved by applying a sine and cosine function to the world-space coordinates (x, z) of each vertex. The amplitude and frequency of the waves are varied by using a noise function based on the vertex position, creating a more natural and less uniform appearance. The calculated wave offsets the y-coordinate of the vertex position, giving the impression of undulating water. The shader ensures the adjustments are perspective-correct by scaling the offset by the w-component of the clip space position, enhancing the realism of the effect.

* **Post-process Camera Overlay**: The water GLSL fragment shader simulates a dynamic water effect by blending textural data with procedural noise. It retrieves color from an albedo texture and modifies it using a fractal brownian motion (fbm) function, which applies cubic interpolation for noise generation across three-dimensional space. This noise influences the texture's brightness, simulating light interaction with moving water surfaces. Additionally, the shader creates a shimmering effect using a complex trigonometric transformation, enhancing the water's visual complexity. This combination of texture manipulation and procedural generation creates a realistic and dynamic water overlay. The lava shader simulates fluid flow across surfaces by computing a noise-based distortion field. It uses a noise function to determine the flow direction and intensity at various points, adjusted dynamically by the shader's time variable, simulating natural fluid movement. This is further processed to compute gradients and influence the flow's directionality. The result is visually represented as a color modulation over the albedo texture, adding a sense of depth and motion to the rendered surface, mimicking the appearance of flowing, viscous material.

## Additional Biomes & Procedurally placed assets (Yuetian Zhao)
* **Additional Biomes**: Incorporate the new block types 'SNOW' and 'SAND' into terrain generation system. Utilize two distinct noise functions, Perlin noise and fbm, to generate detailed maps representing moisture and temperature distributions across your virtual world. These maps serve as dynamic templates, capturing the nuanced variations in moisture levels and thermal gradients that influence biome formations. Leveraging the information from these maps, implement an intelligent biome determination mechanism. This algorithmic approach analyzes the moisture and temperature data at each point of your terrain, allowing it to categorize regions into different biomes such as lush grasslands. By defining specific thresholds and criteria based on environmental factors, system can accurately discern which biome characteristics are prevalent in each area. Once the biomes are identified, integrate them into terrain generation pipeline. As the terrain takes shape. For instance, 'SNOW' blocks should adorn frigid landscapes characterized by low temperatures and high moisture, while 'SAND' blocks find their home in arid expanses with minimal moisture content. Simulating natural ecosystems where different biomes coexist harmoniously.

* **Procedurally placed assets **: Add CACTUS, RED_FLOWER, AND LONG/MID GRASS to the biome, for trees and cactus, use two noise function to determine one region, and in that region, only one tree can exist, for grass and flower, use one perlin noise to determine a position should place asset or not.   
</details>
</details>

 ## [DEMO VIDEO](https://drive.google.com/file/d/14JDHjDfNVG4hfhrKqVo-6rTJybCE39Hd/view?usp=sharing)
|<img src="result/Result (1).jpg" width="100%">|<img src="result/Result (3).jpg" width="100%">|<img src="result/Result (6).jpg" width="100%">|<img src="result/Result (4).jpg" width="100%">|
|:-:|:-:|:-:|:-:|
|Demo|Demo|Demo|Demo|
|<img src="result/AlbedoTexture_MC.png" width="100%">|<img src="result/NormalTexture_MC.png" width="100%">|<img src="result/ShadowTexture_MC.png" width="100%">|<img src="result/LightingTexture_MC.png" width="100%">|
|Albedo|Normal|Shadow|Lighting|


## Post Processing
* **C++ Rasterizer** converts 3D models into 2D images by projecting vertices onto a screen and filling in the resulting shapes (polygons). Implement by iterating over the polygons, converting 3D coordinates to 2D screen space, and using scanline filling or edge functions to color pixels within the polygons.

* **Blinn-Phong** creates specular highlights by using the halfway vector between the light and view directions. Implement by calculating the dot product of this halfway vector and the surface normal, then raising the result to the shininess coefficient, which controls the size of the highlight.

* **Gaussian Blur** applies a kernel matrix based on the Gaussian function across the image to blur it, with the blur extent controlled by the kernel's standard deviation. This averages the colors of each pixel with its neighbors, softening the overall image.

* **MatCap** simulates complex lighting and material effects using a 2D texture. Implement by sampling the MatCap texture with the surface normal vector as coordinates, effectively wrapping the texture around the object based on its geometry.

* **Sobel Filter** detects edges by applying horizontal and vertical convolution matrices to the image, emphasizing regions where there is a high color gradient. Combine these gradients to highlight the edges clearly.

* **Pointilism** is implemented by applying a stippling effect with Worley noise, creating a pattern of small, colored dots that reconstruct the image. Each dot's color corresponds to the underlying image pixel it represents. The use of Worley noise introduces a more natural, irregular pattern to the dot distribution, enhancing the artistic effect. The density and size of the dots can be varied to achieve different levels of abstraction and detail.

* **Toon Shading** quantizes light values into discrete bands to achieve a cartoon-like appearance. Implement by modifying the standard lighting calculation with a step function to create these bands, often adding an outline for a stylized look.

* **Anaglyph** generates a stereo image by combining two differently colored images from slightly offset perspectives. Implement by separating and then recombining the color channels of these images, creating a 3D effect when viewed with color-filtered glasses.


|<img src="Result/rasterizer.jpg" width="100%">|<img src="Result/BlinnPhong.jpg" width="100%">|<img src="Result/GuassinBlur.jpg" width="100%">|<img src="Result/MatCap.jpg" width="100%">|
|:-:|:-:|:-:|:-:|
|C++ Rasterizer|Blinn-Phong|Guassin Blur|MatCap|
|<img src="Result/Sobel.jpg" width="100%">|<img src="Result/pointlism.jpg" width="100%">|<img src="Result/ToonShader.gif" width="100%">|<img src="Result/Anaglyph.gif" width="100%">|
|Sobel Filter|Pointilism|Toon Shader + Perlin Noise|Anaglyph|

# Path Tracing
## Direct Lighting
|<img src="Result/DirectCornellBoxPointLight.png" width="100%">|<img src="Result/DirectCornellBoxSpotLight.png" width="100%">|<img src="Result/DirectCornellBoxTwoLights.png" width="100%">|
|:-:|:-:|:-:|
|Point Light|Spot Light|Area Lights|


## Indirect Lighting
|<img src="Result/cornellBoxNaive.png" width="100%">|<img src="Result/result2.jpg" width="100%">|<img src="Result/result3.jpg" width="100%">|
|:-:|:-:|:-:|
|Cornel Box|BSDF Diffuse|Reflect & Transmit|

## Global Illumination & MIS
* **Global Illumination (GI)** I  harness Global Illumination (GI) in my rendering engine to capture the complex interplay of light within a scene, including both direct and indirect illumination. This approach allows me to achieve lifelike lighting effects, such as soft shadows and color bleeding, which surpass the capabilities of traditional local illumination models by simulating the scattering of light off surfaces.

* **Multiple Importance Sampling (MIS)** A pivotal element in my GI strategy is Multiple Importance Sampling (MIS), which optimizes the rendering process by intelligently combining diverse light sampling strategies. This is vital for efficiently rendering complex lighting scenarios with minimal variance, ensuring smoother textures and more consistent lighting effects.

* **Power Heuristic** In my implementation, I employ the Power Heuristic within the MIS framework to fine-tune the weighting of different sampling strategies. This method ensures an optimal balance between the contributions of direct and indirect lighting, leading to faster convergence and high-quality, realistic renders.

* **BSDF Sampling** I utilize BSDF Sampling to focus on the nuanced material properties of objects in the scene, effectively capturing the way light reflects and transmits based on the Bidirectional Scattering Distribution Function. This technique is especially beneficial for materials with prominent specular or transmission characteristics, ensuring that critical light interactions are accurately rendered.

Conversely, my Light Source (LE) Sampling strategy targets the illumination sources in the scene, prioritizing rays that emanate directly from these lights. This approach is essential for rendering direct lighting effects, particularly from compact or intense light sources, thereby preventing them from being overlooked due to their limited spatial influence.

By integrating these advanced techniques, my engine delivers exceptional realism and efficiency in rendering various lighting conditions, from the subtle interplay of indirect light to the bold contrasts of direct illumination.
|<img src="Result/result5.jpg" width="100%">|<img src="Result/result.jpg" width="100%">|<img src="Result/result9.jpg" width="100%">|
|:-:|:-:|:-:|
|Cornel Box|Microfacet Metal|Envr Map|
|<img src="Result/result8.jpg" width="100%">|<img src="Result/result6.jpg" width="100%">|<img src="Result/result7.jpg" width="100%">|
|Custom Scene|Perfect Mirror|Rough Mirror|


