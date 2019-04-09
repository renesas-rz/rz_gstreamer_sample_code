#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform mat4 mvp_matrix;

attribute vec3 a_position;
attribute vec3 a_color;

varying vec3 v_color;

void main()
{
    // Calculate vertex position in screen space
    gl_Position = mvp_matrix * vec4(a_position.x, a_position.y, a_position.z, 1.0);

    // Pass color to fragment shader
    // Value will be automatically interpolated to fragments inside polygon faces
    v_color = a_color;
}
