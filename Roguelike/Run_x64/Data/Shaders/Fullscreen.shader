<shader name="fullscreen">
    <shaderprogram src="Data/ShaderPrograms/Fullscreen_VS.cso" />
    <shaderprogram src="Data/ShaderPrograms/Fullscreen_PS.cso" />
    <raster src="__default" />
    <sampler src="__point" />
    <blends>
        <blend enable="false">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
</shader>