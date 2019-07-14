<shader name="fullscreen">
    <shaderprogram src="Data/ShaderPrograms/Fullscreen.hlsl">
        <pipelinestages>
            <vertex entrypoint="VertexFunction" />
            <pixel  entrypoint="PixelFunction" />
        </pipelinestages>
    </shaderprogram>
    <raster src="__default" />
    <sampler src="__point" />
    <blends>
        <blend enable="false">
            <color src="src_alpha" dest="inv_src_alpha" op="add" />
        </blend>
    </blends>
</shader>