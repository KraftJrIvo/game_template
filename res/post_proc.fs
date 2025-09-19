#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform float time;       

out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;
    uv.x += 0.02 * sin(time + uv.y * 10.0);
    uv.y += 0.02 * cos(time + uv.x * 10.0);
    vec4 color = texture2D(texture0, uv);
    float brightness = 0.5 + 0.5 * sin(time);
    color.rgb *= brightness + 0.5;
    finalColor = color * colDiffuse;
}
