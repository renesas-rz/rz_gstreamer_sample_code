#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

varying vec3 v_color;

void main()
{
    // Set color
    gl_FragColor = vec4(v_color.x, v_color.y, v_color.z, 1.0);
}
