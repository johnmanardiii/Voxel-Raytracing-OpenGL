# Voxel-Raytracing-OpenGL
![GIF of Denoised Scene](https://github.com/johnmanardiii/Voxel-Raytracing-OpenGL/blob/main/images/denoised_swinging.gif?raw=true)

## What is this project?
Done as part of CSC 572, a graduate class at Cal Poly SLO, I implemented a project demonstrating global illumination using a hybrid rasterizer/raytracing approach. The implementation renders the scene into a 3D texture (voxels), then in a deferred render pass, the ambient light is calculated by raymarching the 3D texture for indirect lighting using a randomized sphere for directions to gather samples from, similar to the technique used to achieve SSAO. The scene is then denoised using a blur pass where only samples that are at about similar depth/normal values are blurred together.

This is one of my favorite projects and I learned probably the most about the render pipeline while doing this project.

[Oren Erlich](https://www.linkedin.com/in/orenerlich/overlay/contact-info/) helped with denoising the scene and we used his deferred rendering scene as a jumping point for the project. After this project, he did work on this topic for his senior project in DirectX and compared his results to other hybrid raytracing global illumination techniques.

## Voxelization of the scene

## Raytracing the scene
![GIF of Raytracing results](https://github.com/johnmanardiii/Voxel-Raytracing-OpenGL/blob/main/images/swinging_light.gif?raw=true)


## Denoising the scene

![GIF of Raytracing results](https://github.com/johnmanardiii/Voxel-Raytracing-OpenGL/blob/main/images/raytrace_full.png?raw=true)

For the denoising, [Oren Erlich](https://www.linkedin.com/in/orenerlich/) and I worked on the shader for blurring the results conditionally on their proximity in depth and normals. Oren wrote the blur shader and I helped debug a lot of issues with the noise needing to be weighted (colors summing to 1 and increasing with the number of blur samples).
