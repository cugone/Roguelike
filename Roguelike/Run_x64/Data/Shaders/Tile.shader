<shader name="tile">
    <shaderprogram src="Data/ShaderPrograms/Tile_VS.cso" />
    <shaderprogram src="Data/ShaderPrograms/Tile_PS.cso" />
    <shaderprogram src="Data/ShaderPrograms/Tile_GS.cso" />
    <raster src="__solid" />
    <sampler src="__point" />
    <blends>
        <blend enable="true">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
    <depth enable="false" writable="false" />
    <stencil enable="false" readable="false" writable="false" />
</shader>