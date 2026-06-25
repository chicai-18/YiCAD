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
    gl_Position = u_MV * vec4(pos, 1.0f);	// no project
    para_vshader = para;
    total_length_vshader = total_length;
}

// ---------------------------------
#shader geometry
#version 430
#define GEOMETRY 
#shader include "common.inl"

// need 3 segments at least, because we check the segment type (start, end, mid) by position 
layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 4) out;

in float para_vshader[];
in float total_length_vshader[];

out float para_gshader;
out float dist_to_center_gshader;
out float total_length_gshader;

void emitVertices(float para, float para_v)
{
    para_gshader = para;
    dist_to_center_gshader = para_v;
    total_length_gshader = total_length_vshader[1];
}

void main()
{
    vec4 start_in_view = gl_in[1].gl_Position;
	vec4 end_in_view = gl_in[2].gl_Position;
    vec4 pt0 = gl_in[0].gl_Position;
    vec4 pt1 = start_in_view;
    vec4 pt2 = end_in_view;
    vec4 pt3 = gl_in[3].gl_Position;

    vec3 dir = normalize( end_in_view.xyz - start_in_view.xyz);	// in view CS
	mat4 rMat=buildRotateZ(PI/2.0);
	vec4 normal = rMat*vec4(dir,1.0f);	// in view CS

    float size_per_pixel = get_size_per_pixel();
	float width_pixel = get_line_width_pixel();

    // determine the direction of each of the 3 segments (previous, current, next)
    vec2 v0 = normalize(pt1.xy - pt0.xy);
    vec2 v1 = normalize(pt2.xy - pt1.xy);
    vec2 v2 = normalize(pt3.xy - pt2.xy);
    // determine the normal of each of the 3 segments (previous, current, next)
    vec2 n0 = vec2(-v0.y, v0.x);
    vec2 n1 = vec2(-v1.y, v1.x);
    vec2 n2 = vec2(-v2.y, v2.x);
    // determine miter lines by averaging the normals of the 2 segments
    vec2 miter_a = normalize(n0 + n1);    // miter at start of current segment
    vec2 miter_b = normalize(n1 + n2);    // miter at end of current segment
    // determine the length of the miter by projecting it onto normal and then inverse it
    float half_width_pixel = width_pixel / 2.0f;
    float half_width = size_per_pixel * half_width_pixel;
    float length_a = half_width / dot(miter_a, n1);
    float length_b = half_width / dot(miter_b, n1);
    vec3 p1, p2, p3, p4;
    float para_p1, para_p2, para_p3, para_p4;
    float dy_p1, dy_p2, dy_p3, dy_p4;
    float paraStart = para_vshader[1];
    float paraEnd = para_vshader[2];
    bool is_start = false;
    bool is_end = false;
    float tol = 1e-5;

    vec3 offset = vec3(miter_a * length_a, 0.0f);
    float para_offset = dot(- miter_a * length_a, v1);
    p1 = vec3(u_P * vec4(start_in_view.xyz + offset, 1.0f));
    p2 = vec3(u_P * vec4(start_in_view.xyz - offset, 1.0f));
    para_p1 = paraStart - para_offset;
    para_p2 = paraStart + para_offset;
    dy_p1 = half_width;
    dy_p2 = - half_width;

    offset = vec3(miter_b * length_b, 0.0f);
    para_offset = dot(- miter_b * length_b, v1);
    p3 = vec3(u_P * vec4(end_in_view.xyz + offset, 1.0f));
    p4 = vec3(u_P * vec4(end_in_view.xyz - offset, 1.0f));
    para_p3 = paraEnd - para_offset;
    para_p4 = paraEnd + para_offset;
    dy_p3 = half_width;
    dy_p4 = - half_width;

    gl_Position=vec4(p1, 1.0f);
    emitVertices(para_p1, dy_p1);
    EmitVertex();
    gl_Position=vec4(p2, 1.0f);
    emitVertices(para_p2, dy_p2);
    EmitVertex();
    gl_Position=vec4(p3, 1.0f);
    emitVertices(para_p3, dy_p3);
    EmitVertex();
    gl_Position=vec4(p4, 1.0f);
    emitVertices(para_p4, dy_p4);
    EmitVertex();
    EndPrimitive();
}


// ---------------------------------
#shader fragment
#version 430 
#define FRAGMENT 
#shader include "common.inl"

in float para_gshader;
in float dist_to_center_gshader;
in float total_length_gshader;

//out vec4 color;

void main()
{
    vec4 the_color = get_color_by_type();
    float line_width_pixel = get_line_width_pixel();
    float size_per_pixel = get_size_per_pixel();
    float half_line_width_pixel = line_width_pixel / 2.0f;
    float half_line_width_world = size_per_pixel * half_line_width_pixel;
    float radius = half_line_width_world;
    float para = para_gshader;
    float dy = dist_to_center_gshader;
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
        if(total_length_gshader < u_dash_sum_length)
        {
            color = the_color;
        }
        else
        {
            float count = ceil(total_length_gshader / u_dash_sum_length);
            if(count == 1.0f)
            {
                count += 1.0f;  // at least 2.0 
            }
            float dash_sum_length_new = total_length_gshader / count;
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
                set_side_color(the_color, zero_para_delta, dy, radius);
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
                    if(!color_set)
                    {
                        discard;
                    }
                }
            }
        }
    }
}
