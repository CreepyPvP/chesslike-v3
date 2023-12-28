.PHONY: clean 

shader: shader/pbr_frag.spv shader/staticv.spv shader/skinnedv.spv

shader/pbr_frag.spv: shader/pbr.frag
	glslc shader/pbr.frag -o shader/pbr_frag.spv

shader/staticv.spv: shader/pbr.vert
	glslc shader/pbr.vert -o shader/staticv.spv

shader/skinnedv.spv: shader/skinned.vert
	glslc shader/skinned.vert -o shader/skinnedv.spv

clean:
	del shader\*.spv
