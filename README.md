# Demiurge

A heightfield editor based on spherical geometry, written in C++ using OpenGL.
Includes typical raster editing tools such as brushes, selections and filters like blur and different fractal noise generators.
Also includes more advanced simulation based tools to perform erosion and generate realistic terrain.
The terrain can be shown using various map projections, allowing for drawing directly onto the projection and adjusting for any distortion.

Editing terrain on a globe, using a brush:
![](https://raw.githubusercontent.com/Kuhlwein/demiurge/master/misc/Peek%202020-08-20%2000-41.gif?token=AFBKK7MUUGDPX4D4QGH36K27I3UYI)

Terrain generation using advanced noise functions and simulations based on tectonic uplift and fluvial erosion:
![](https://raw.githubusercontent.com/Kuhlwein/demiurge/master/misc/erosion.png?token=AFBKK7O5TRANXIKAHFPH2BK7I3U2S)

Simulation of ocean currents using GPU-accelerated fluid dynamics on a spherical surface:
![](https://raw.githubusercontent.com/Kuhlwein/Demiurge/master/misc/ocean_sim.png)

Work in progress climate simulation, here surface temperature throughout the year:
![](https://github.com/Kuhlwein/Demiurge/blob/master/misc/climate_sim_wip.gif)

Editing the map using different map projections, here Goode Homolosine and Robinson:
![](https://raw.githubusercontent.com/Kuhlwein/demiurge/master/misc/im1.png?token=AFBKK7JLP53GTO3PMDEM2AC7I3VCW)
![](https://raw.githubusercontent.com/Kuhlwein/demiurge/master/misc/im2.png?token=AFBKK7ITYTPBDLJOJTHSOXC7I3VC6)
