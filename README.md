# OpenGL-RealTime-Offline-Rendering

* Ruijun(Daniel) Zhong
    * [LinkedIn](https://www.linkedin.com/in/daniel-z-73158b152/)    
    * [Personal Website](https://www.danielzhongportfolio.com/)
* Tested on: 
  * Core(TM) i7-12700K 3.61 GHz 32.0 GB, NVIDIA GeForce RTX 3070 Ti (personal computer)   
    * Maya 2022, Houdini 20.0.590, Visua Studio 2022



# Real Time Rendering
* **C++ Rasterizer** converts 3D models into 2D images by projecting vertices onto a screen and filling in the resulting shapes (polygons). Implement by iterating over the polygons, converting 3D coordinates to 2D screen space, and using scanline filling or edge functions to color pixels within the polygons.

* **Blinn-Phong** creates specular highlights by using the halfway vector between the light and view directions. Implement by calculating the dot product of this halfway vector and the surface normal, then raising the result to the shininess coefficient, which controls the size of the highlight.

* **Gaussian Blur** applies a kernel matrix based on the Gaussian function across the image to blur it, with the blur extent controlled by the kernel's standard deviation. This averages the colors of each pixel with its neighbors, softening the overall image.

* **MatCap** simulates complex lighting and material effects using a 2D texture. Implement by sampling the MatCap texture with the surface normal vector as coordinates, effectively wrapping the texture around the object based on its geometry.

* **Sobel Filter** detects edges by applying horizontal and vertical convolution matrices to the image, emphasizing regions where there is a high color gradient. Combine these gradients to highlight the edges clearly.

* **Pointilism** is implemented by applying a stippling effect with Worley noise, creating a pattern of small, colored dots that reconstruct the image. Each dot's color corresponds to the underlying image pixel it represents. The use of Worley noise introduces a more natural, irregular pattern to the dot distribution, enhancing the artistic effect. The density and size of the dots can be varied to achieve different levels of abstraction and detail.

* **Toon Shading** quantizes light values into discrete bands to achieve a cartoon-like appearance. Implement by modifying the standard lighting calculation with a step function to create these bands, often adding an outline for a stylized look.

* **Anaglyph** generates a stereo image by combining two differently colored images from slightly offset perspectives. Implement by separating and then recombining the color channels of these images, creating a 3D effect when viewed with color-filtered glasses.


|<img src="rasterizer.jpg" width="100%">|<img src="BlinnPhong.jpg" width="100%">|<img src="GuassinBlur.jpg" width="100%">|<img src="MatCap.jpg" width="100%">|
|:-:|:-:|:-:|:-:|
|C++ Rasterizer|Blinn-Phong|Guassin Blur|MatCap|
|<img src="Sobel.jpg" width="100%">|<img src="pointlism.jpg" width="100%">|<img src="ToonShader.gif" width="100%">|<img src="Anaglyph.gif" width="100%">|
|Sobel Filter|Pointilism|Toon Shader + Perlin Noise|Anaglyph|

# Path Tracing
## Direct Lighting
|<img src="DirectCornellBoxPointLight.png" width="100%">|<img src="DirectCornellBoxSpotLight.png" width="100%">|<img src="DirectCornellBoxTwoLights.png" width="100%">|
|:-:|:-:|:-:|
|Point Light|Spot Light|Area Lights|


## Indirect Lighting
|<img src="cornellBoxNaive.png" width="100%">|<img src="result2.jpg" width="100%">|<img src="result3.jpg" width="100%">|
|:-:|:-:|:-:|
|Cornel Box|BSDF Diffuse|Reflect & Transmit|

## Global Illumination & MIS
* **Global Illumination (GI)** I  harness Global Illumination (GI) in my rendering engine to capture the complex interplay of light within a scene, including both direct and indirect illumination. This approach allows me to achieve lifelike lighting effects, such as soft shadows and color bleeding, which surpass the capabilities of traditional local illumination models by simulating the scattering of light off surfaces.

* **Multiple Importance Sampling (MIS)** A pivotal element in my GI strategy is Multiple Importance Sampling (MIS), which optimizes the rendering process by intelligently combining diverse light sampling strategies. This is vital for efficiently rendering complex lighting scenarios with minimal variance, ensuring smoother textures and more consistent lighting effects.

* **Power Heuristic** In my implementation, I employ the Power Heuristic within the MIS framework to fine-tune the weighting of different sampling strategies. This method ensures an optimal balance between the contributions of direct and indirect lighting, leading to faster convergence and high-quality, realistic renders.

* **BSDF Sampling** I utilize BSDF Sampling to focus on the nuanced material properties of objects in the scene, effectively capturing the way light reflects and transmits based on the Bidirectional Scattering Distribution Function. This technique is especially beneficial for materials with prominent specular or transmission characteristics, ensuring that critical light interactions are accurately rendered.

Conversely, my Light Source (LE) Sampling strategy targets the illumination sources in the scene, prioritizing rays that emanate directly from these lights. This approach is essential for rendering direct lighting effects, particularly from compact or intense light sources, thereby preventing them from being overlooked due to their limited spatial influence.

By integrating these advanced techniques, my engine delivers exceptional realism and efficiency in rendering various lighting conditions, from the subtle interplay of indirect light to the bold contrasts of direct illumination.
|<img src="result5.jpg" width="100%">|<img src="result.jpg" width="100%">|<img src="result9.jpg" width="100%">|
|:-:|:-:|:-:|
|Cornel Box|Microfacet Metal|Envr Map|
|<img src="result8.jpg" width="100%">|<img src="result6.jpg" width="100%">|<img src="result7.jpg" width="100%">|
|Custom Scene|Perfect Mirror|Rough Mirror|


## Cook-Torrance BRDF(point lights)
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

Crucial for real-time graphics, the Cook-Torrance BRDF strikes a balance between visual realism and computational efficiency, enabling complex lighting effects in interactive applications without extensive resource demands.
|<img src="default.png" width="100%">|<img src="fullMetal.png" width="100%">|<img src="fullPlastic.png" width="100%">|
|:-:|:-:|:-:|
|default|Full Metal|Full Plastic|
|<img src="lowRough.png" width="100%">|<img src="highRough.png" width="100%">|<img src="XC.jpg" width="100%">|
|Low Rough|High Rough|Procedural Texture|