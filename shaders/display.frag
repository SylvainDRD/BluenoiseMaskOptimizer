#version 430 core

#define DIMENSION 1337

out vec4 color;

uniform sampler2DArray mask;


void main() {
    ivec3 coords = ivec3(ivec2(gl_FragCoord.xy), 0);
    vec4 value = texelFetch(mask, coords, 0);

    if(DIMENSION == 1)
        color = vec4(value.x);
    else 
        color = value;
        
}
