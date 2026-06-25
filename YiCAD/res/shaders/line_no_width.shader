#shader vertex
#version 430
#define VERTEX 
#shader include "common.inl"

layout(location = 0) in vec3 pos;
layout(location = 1) in float para; //start parameter 
layout(location = 2) in float total_length;

out float para_vshader;
out float total_length_vshader;

void main()
{
    gl_Position = u_MVP * vec4(pos, 1.0f);	
    para_vshader = para;
    total_length_vshader = total_length;
}

// ---------------------------------
#shader fragment
#version 430 
#define FRAGMENT 
#shader include "common.inl"

in float para_vshader;
in float total_length_vshader;

void main()
{
    vec4 the_color = get_color_by_type();
    float line_width_pixel = get_line_width_pixel();
    float size_per_pixel = get_size_per_pixel();
    float half_line_width_pixel = line_width_pixel / 2.0f;
    float half_line_width_world = size_per_pixel * half_line_width_pixel;
    float radius = half_line_width_world * 2.0f; // larger than line.shader, too small may cause some points missing
    float para = para_vshader;

    // no dash
    if(u_dashes_size == 0)
    {
        color = the_color;
    }
    // with dash
    else
    {
        bool is_dot = u_dash_para_pairs_num == 0;
        bool color_set = false;
        // dot style
        if(is_dot)
        {
            // total length less than dash sum length, display two dots
            if(total_length_vshader < u_dash_sum_length)
            {
                if(abs(para_vshader) < radius || abs(para_vshader - total_length_vshader) < radius)
                {
                    color = the_color;
                }
                else
                {
                    discard;
                }
                color_set = true;
            }
        }
        // not dot style
        else
        {
            // total length less than dash sum length, display no dash
            if(total_length_vshader < u_dash_sum_length)
            {
                color = the_color;
                color_set = true;
            }
        }

        if(!color_set)
        {
            float factor = total_length_vshader / u_dash_sum_length;
            float factor_fract = fract(factor);
            float factor_num = fract(factor);
            float para_offset1 = factor_fract * 0.5f * u_dash_sum_length;   // parameter between (para_offset1, para_offset2) display line style, others display solid line or space
            float para_offset2 = total_length_vshader - para_offset1;

            // begin and end outside, display semicicle
            if(para < 0.0f || para > total_length_vshader)
            {
                color = the_color;
            }
            // begin segment, display solid line or space
            else if(para >= 0.0f && para <= para_offset1)
            {
                color = the_color;
            }
            // end segment, display solid line or space
            else if(para >= para_offset2 && para <= total_length_vshader)
            {
                color = the_color;
            }
            // middle segment, display line style
            else
            {
                float para_display = para - para_offset1; // >=0
                if(u_dash_para_pairs_num != 0)
                {
                    // start form half of first solid line
                    para_display += (u_dash_para_pairs[0] + u_dash_para_pairs[1]) / 2.0f;
                }
                float para_display_round = mod(para_display, u_dash_sum_length);

                // zero point check
                bool is_zero = set_zero_point_no_width(the_color, para_display_round, radius);
                // is dash line
                if(!is_zero)
                {
                    set_line_blank_no_width(the_color, para_display_round, radius);
                }
            }
        }
    }
}
