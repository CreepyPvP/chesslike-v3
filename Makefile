shader: frag.spv vert.spv

frag.spv: shader/pbr.frag
	glslc shader/pbr.frag -o frag.spv

vert.spv: shader/pbr.vert
	glslc shader/pbr.vert -o vert.spv
