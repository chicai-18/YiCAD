#shader vertex
#version 430

uniform mat4 u_MVP;
uniform vec4 u_Color;
layout(location = 0) in vec3 pos;
layout(location = 1) in float para;

out float para_vshader;

void main()
{
  gl_Position = u_MVP *vec4(pos, 1.0); 
  para_vshader = para;
}

//-------------------------------------------
#shader fragment
#version 430
#define FRAGMENT 
#shader include "common.inl"

in float para_vshader;

void main() 
{
    // no dash
    if(u_dashes_size == 0)
    {
      color = u_Color;
    }
    // with dash
    else
    {
      float size_per_pixel = get_size_per_pixel();
      float sum_len_per_dash = u_dash_sum_length * size_per_pixel;  // u_dash_sum_length is pixel coordinate
      float para_round = mod(para_vshader, sum_len_per_dash);
      int i=0;
      for(;i<u_dash_para_pairs_num;i++)
      {
        float start_para = u_dash_para_pairs[2*i] * size_per_pixel;
        float end_para = u_dash_para_pairs[2*i + 1] * size_per_pixel;
        if(para_round < start_para)   // following parameter pairs are larger, no need check
        {
            break;
        }
        bool isBetween = para_round >= start_para && para_round <= end_para;
        if(isBetween)
        {
            color = u_Color;
            break;
        }
      }
    }
} 

