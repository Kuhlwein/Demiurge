# Demiurge

A heightfield editor based on spherical geometry, written in C++ and glsl.
Includes typical raster editing tools such as brushes, selections and filters like blur and different fractal noise generators.
Also includes more advanced simulation based tools to perform erosion and generate realistic terrain.
The terrain can be shown using various map projections, allowing for drawing directly onto the projection and adjusting for any distortion.

Editing terrain on a globe, using a brush:
![](https://raw.githubusercontent.com/Kuhlwein/demiurge/master/misc/Peek%202020-08-20%2000-41.gif?token=AFBKK7MUUGDPX4D4QGH36K27I3UYI)

Terrain generation using advanced noise functions and simulations based on tectonic uplift and fluvial erosion:
![](https://raw.githubusercontent.com/Kuhlwein/demiurge/master/misc/erosion.png?token=AFBKK7O5TRANXIKAHFPH2BK7I3U2S)

Editing the map using different map projections, here Goode Homolosine and Robinson:
![](https://raw.githubusercontent.com/Kuhlwein/demiurge/master/misc/im1.png?token=AFBKK7JLP53GTO3PMDEM2AC7I3VCW)
![](https://raw.githubusercontent.com/Kuhlwein/demiurge/master/misc/im2.png?token=AFBKK7ITYTPBDLJOJTHSOXC7I3VC6)
