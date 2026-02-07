#version 450

// Input vertex attributes matching the Vertex struct layout
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

// Output to fragment shader
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPos, 1.0);
    
    // Generate color based on position - this will consume the inPos attribute
    fragColor = vec3(
        inPos.x + 0.5,  // Map X [-0.5, 0.5] to [0.0, 1.0] (red)
        inPos.y + 0.5,  // Map Y [-0.5, 0.5] to [0.0, 1.0] (green)  
        1.0             // Always full blue for visibility
    );
}