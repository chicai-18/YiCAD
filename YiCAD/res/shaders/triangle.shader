#shader vertex
#version 430
#define VERTEX 
#shader include "common.inl"

layout(location = 0) in vec3 pos;

void main()
{
  gl_Position = u_MVP *vec4(pos, 1.0); 
}

// ---------------------------------
#shader fragment
#version 430 
#define FRAGMENT 
#shader include "common.inl"

void main() 
{
   color = get_color_by_type();
} 