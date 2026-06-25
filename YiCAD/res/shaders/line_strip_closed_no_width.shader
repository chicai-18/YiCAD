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
    float radius = half_line_width_world * 2.0f;
    float para = para_vshader;
    float tol = 1e-5;

    // no dash
    if(u_dashes_size == 0)
    {
        color = the_color;
    }
    // with dash
    else
    {
        // total length less than dash sum length, display no dash
        if(total_length_vshader < u_dash_sum_length)
        {
            color = the_color;
        }
        else
        {
            float count = ceil(total_length_vshader / u_dash_sum_length);
            if(count == 1.0f)
            {
                count += 1.0f;  // at least 2.0 
            }
            float dash_sum_length_new = total_length_vshader / count;
            float scale = dash_sum_length_new / u_dash_sum_length;
            float para_round = mod(para, dash_sum_length_new);

            // zero point check
            bool is_zero = false;
            float zero_para_delta = 0.0f;
            int i = 0;
            for(i=0;i<u_dash_zero_num;i++)
            {
                zero_para_delta = abs(u_dash_zero_paras[i] * scale - para_round);
                if(zero_para_delta < radius)
                {
                    is_zero = true;
                    break;
                }
                if(u_dash_zero_paras[i] < tol || abs(u_dash_zero_paras[i] - u_dash_sum_length)  < tol)  //0.0
                {
                    zero_para_delta = abs(dash_sum_length_new - para_round);
                    if(zero_para_delta < radius)
                    {
                        is_zero = true;
                        break;
                    }
                    zero_para_delta = abs(para_round);
                    if(zero_para_delta < radius)
                    {
                        is_zero = true;
                        break;
                    }
                }
            }
            // is zero point
            if(is_zero)
            {
                color = the_color;
            }
            // is dash line
            else
            {
                bool color_set = false;
                // check in line
                i=0;
                for(;i<u_dash_para_pairs_num;i++)
                {
                    float start_para = u_dash_para_pairs[2*i] * scale;
                    float end_para = u_dash_para_pairs[2*i + 1] * scale;
                    if(para_round < start_para)   // following parameter pairs are larger, no need check
                    {
                        break;
                    }
                    bool isBetween = para_round >= start_para && para_round <= end_para;
                    if(isBetween)
                    {
                        color = the_color;
                        color_set = true;
                        break;
                    }
                }

                // check blank side
                if(!color_set)
                {
                    i = 0;
                    for(;i<u_dash_blank_para_pairs_num;i++)
                    {
                        float start_para = u_dash_blank_para_pairs[2*i] * scale;
                        float end_para = u_dash_blank_para_pairs[2*i + 1] * scale;
                        if(para_round < start_para)   // following parameter pairs are larger, no need check
                        {
                            break;
                        }
                        bool blank_left_side = para_round > start_para && para_round - start_para < radius;
                        bool blank_right_side = para_round < end_para && end_para - para_round < radius;
                        if(blank_left_side || blank_right_side)
                        {
                            color = the_color;
                            color_set = true;
                            break;
                        }
                    }
                    if(!color_set)
                    {
                        discard;
                    }
                }
            }
        }
    }
}
