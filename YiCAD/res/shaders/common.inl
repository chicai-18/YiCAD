#define PI 3.1415926
// antialiasing width, usually 3 pixels
#define HIGHLIGHT_WIDTH 4.0f
#define TOL 1e-5f

uniform mat4 u_MVP;
uniform mat4 u_MV;
uniform mat4 u_P;
uniform vec4 u_Color;
uniform float u_lineWidth;	// pixel value
uniform float u_dashes[64];
uniform int u_dashes_size; 
uniform float u_dash_sum_length;
uniform float u_dash_zero_paras[64];    // zero parameters  
uniform float u_dash_para_pairs[128];   // no blank parameter pairs
uniform float u_dash_blank_para_pairs[128];   // blank parameter pairs
uniform int u_dash_para_pairs_num;
uniform int u_dash_blank_para_pairs_num;
uniform int u_dash_zero_num;
uniform vec2 u_viewportSize;
uniform vec4 u_selectedColor;
uniform vec4 u_highlightColor;
uniform bool u_isSelected;
uniform bool u_isHighlighted;

#if defined(VERTEX) || defined(FRAGMENT) || defined(GEOMETRY)
    mat4 buildRotateZ(float rad)
    {
        mat4 zrot = mat4(cos(rad), sin(rad), 0.0, 0.0,
                        -sin(rad), cos(rad), 0.0, 0.0,
                        0.0, 0.0, 1.0, 0.0,
                        0.0, 0.0, 0.0, 1.0);
        return zrot;
    }

    float get_size_per_pixel()
    {
        float cell00 = u_P[0][0];
        float width_in_view_CS = 2.0 / cell00;
        return width_in_view_CS / u_viewportSize.x;
    }

    
    float get_line_width_pixel()
    {
        float width_pixel = 0.0f;
        if(u_isSelected || u_isHighlighted)
        {
            width_pixel = u_lineWidth + HIGHLIGHT_WIDTH;
        }
        else
        {
            width_pixel = u_lineWidth;
        }
        return width_pixel;
    }
#endif

#if defined(GEOMETRY)
    // check whether v is between min and max. is between  get rectangle, less than min get triangle, more than max get pentagon
    void test_value_geometry(float v, float min, float max)
    {
        float w = 0.5f;
        if(v>=min && v<=max)
        {
            // rectangle
            gl_Position=vec4(0.0f, 0.0f, 0.0f, 1.0f);
            EmitVertex();
            gl_Position=vec4(0.0f, -w, 0.0f, 1.0f);
            EmitVertex();
            gl_Position=vec4(w, 0.0f, 0.0f, 1.0f);
            EmitVertex();
            gl_Position=vec4(w, -w, 0.0f, 1.0f); 
            EmitVertex();
            EndPrimitive();
        }
        else if(v<min)
        {
            // triangle
            gl_Position=vec4(0.0f, 0.0f, 0.0f, 1.0f);
            EmitVertex();
            gl_Position=vec4(0.0f, -w, 0.0f, 1.0f);
            EmitVertex();
            gl_Position=vec4(w, 0.0f, 0.0f, 1.0f);
            EmitVertex();
            EndPrimitive();
        }
        else
        {
            // pentagon
            gl_Position = vec4(w/2.0f, w, 0.0f, 1.0f);
            EmitVertex();
            gl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);
            EmitVertex();
            gl_Position = vec4(w, 0.0f, 0.0f, 1.0f);
            EmitVertex();
            gl_Position = vec4(0.0f, -w, 0.0f, 1.0f);
            EmitVertex();
            gl_Position = vec4(w,-w,0.0f, 1.0f);
            EmitVertex();
            EndPrimitive();
        }
    }

#endif 


#if defined(FRAGMENT)

    // output color
    out vec4 color;

    // check whether v is between min and max. is between  get green, less than min get red, more than max get blue
    vec4 test_value_flagment(float v, float min, float max)
    {
        vec4 color;
        if(v>=min && v<=max)
        {
            color = vec4(0.0, 1.0, 0.0, 1.0);
        }
        else if(v<min)
        {
            color = vec4(1.0, 0.0, 0.0 ,1.0);
        }
        else
        {
            color = vec4(0.0, 0.0, 1.0, 1.0);
        }
        return color;
    }

    // get the select color, or highlight color or entity color. 
    vec4 get_color_by_type()
    {
        vec4 the_color;
        if(u_isSelected)
        {
            the_color = u_selectedColor;
        }
        else if(u_isHighlighted)
        {
            the_color = u_highlightColor;
        }
        else
        {
            the_color = u_Color;
        }
        return the_color;
    }

    void set_side_color(vec4 the_color, float para_delta, float dy, float radius)
    {
        if(para_delta * para_delta + dy * dy < radius * radius)
        {
            color = the_color;
        }
        else
        {
            discard;
        }
    }

    bool set_zero_point(vec4 the_color, float para_round, float dy, float radius)
    {
        bool is_zero = false;
        float zero_para_delta = 0.0f;
        int i = 0;
        for(i=0;i<u_dash_zero_num;i++)
        {
            zero_para_delta = abs(u_dash_zero_paras[i] - para_round);
            if(zero_para_delta < radius)
            {
                is_zero = true;
                break;
            }
            if(u_dash_zero_paras[i] < TOL || abs(u_dash_zero_paras[i] - u_dash_sum_length)  < TOL)  //0.0
            {
                zero_para_delta = abs(u_dash_sum_length - para_round);
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
            set_side_color(the_color, zero_para_delta, dy, radius);
        }
        return is_zero;
    } 

    bool set_zero_point_no_width(vec4 the_color, float para_round, float radius)
    {
        bool is_zero = false;
        float zero_para_delta = 0.0f;
        int i = 0;
        for(i=0;i<u_dash_zero_num;i++)
        {
            zero_para_delta = abs(u_dash_zero_paras[i] - para_round);
            if(zero_para_delta < radius)
            {
                is_zero = true;
                break;
            }
            if(u_dash_zero_paras[i] < TOL || abs(u_dash_zero_paras[i] - u_dash_sum_length)  < TOL)  //0.0
            {
                zero_para_delta = abs(u_dash_sum_length - para_round);
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
        return is_zero;
    } 

    void set_line_blank(vec4 the_color, float para_round, float dy, float radius)
    {
        bool color_set = false;
        // check in line
        int i=0;
        for(;i<u_dash_para_pairs_num;i++)
        {
            float start_para = u_dash_para_pairs[2*i];
            float end_para = u_dash_para_pairs[2*i + 1];
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
                float start_para = u_dash_blank_para_pairs[2*i];
                float end_para = u_dash_blank_para_pairs[2*i + 1];
                if(para_round < start_para)   // following parameter pairs are larger, no need check
                {
                    break;
                }
                bool blank_left_side = para_round > start_para && para_round - start_para < radius;
                bool blank_right_side = para_round < end_para && end_para - para_round < radius;
                if(blank_left_side || blank_right_side)
                {
                    float delta_para = 0.0f;
                    if(blank_left_side)
                    {
                        delta_para = para_round - start_para;
                    }
                    else
                    {
                        delta_para = end_para - para_round;
                    }
                    color_set = true;
                    set_side_color(the_color, delta_para, dy, radius);
                    break;
                }
            }
        }
    }

    void set_line_blank_no_width(vec4 the_color, float para_round, float radius)
    {
        bool color_set = false;
        // check in line
        int i=0;
        for(;i<u_dash_para_pairs_num;i++)
        {
            float start_para = u_dash_para_pairs[2*i];
            float end_para = u_dash_para_pairs[2*i + 1];
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
                float start_para = u_dash_blank_para_pairs[2*i];
                float end_para = u_dash_blank_para_pairs[2*i + 1];
                if(para_round < start_para)   // following parameter pairs are larger, no need check
                {
                    break;
                }
                bool blank_left_side = para_round > start_para && para_round - start_para < radius;
                bool blank_right_side = para_round < end_para && end_para - para_round < radius;
                if(blank_left_side || blank_right_side)
                {
                    color_set = true;
                    color = the_color;
                    break;
                }
            }
        }
    }


#endif 