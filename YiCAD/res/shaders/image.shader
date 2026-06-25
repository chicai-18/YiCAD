#shader vertex
#version 430
#define VERTEX

uniform mat4 u_MVP;
uniform mat4 u_MV;
uniform mat4 u_P;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texCoord;

out vec2 v_TexCoord;

void main()
{
    gl_Position = u_MVP * vec4(pos, 1.0);
    v_TexCoord = texCoord;
}

// ---------------------------------
#shader fragment
#version 430
#define FRAGMENT

uniform sampler2D u_Texture;
uniform vec4 u_Color;
uniform vec4 u_selectedColor;
uniform vec4 u_highlightColor;
uniform bool u_isSelected;
uniform bool u_isHighlighted;

in vec2 v_TexCoord;
out vec4 color;

vec4 get_color_by_type()
{
    vec4 the_color;
    if (u_isSelected)
    {
        the_color = u_selectedColor;
    }
    else if (u_isHighlighted)
    {
        the_color = u_highlightColor;
    }
    else
    {
        the_color = u_Color;
    }
    return the_color;
}

void main()
{
    vec4 texColor = texture(u_Texture, v_TexCoord);
    vec4 entityColor = get_color_by_type();
    color = texColor * entityColor;
}
