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

    // if points are coincident, discard this primitive
    if(length(vec3(pt2 - pt1)) < TOL || length(vec3(pt1 - pt0)) < TOL || length(vec3(pt3 - pt2)) < TOL)
    {
        return;
    }

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
    if(dot(v0, v1) < -(1.0f - tol)) // start segment
    {
        is_start = true;
        vec3 new_start_in_view = vec3(start_in_view) - dir * half_width;
        vec3 offset = vec3(normal.xy * half_width, 0.0f);
        p1 = vec3(u_P * vec4(new_start_in_view + offset, 1.0f));
        p2 = vec3(u_P * vec4(new_start_in_view - offset, 1.0f));
        para_p1 = paraStart - half_width;
        para_p2 = paraStart - half_width;
        dy_p1 = half_width;
        dy_p2 = - half_width;
    }
    else
    {
        vec3 offset = vec3(miter_a * length_a, 0.0f);
        float para_offset = dot(- miter_a * length_a, v1);
        p1 = vec3(u_P * vec4(start_in_view.xyz + offset, 1.0f));
        p2 = vec3(u_P * vec4(start_in_view.xyz - offset, 1.0f));
        para_p1 = paraStart - para_offset;
        para_p2 = paraStart + para_offset;
        dy_p1 = half_width;
        dy_p2 = - half_width;
    }
    if(dot(v1, v2) < -(1.0f - tol)) // end segment
    {
        //miter_b = n1;
        is_end = true;
        //length_b = half_width * sqrt(2.0f);
        vec3 new_end_in_view = vec3(end_in_view) + dir * half_width;
        vec3 offset = vec3(normal.xy * half_width, 0.0f);
        p3 = vec3(u_P * vec4(new_end_in_view + offset, 1.0f));
        p4 = vec3(u_P * vec4(new_end_in_view - offset, 1.0f));
        para_p3 = paraEnd + half_width;
        para_p4 = paraEnd + half_width;
        dy_p3 = half_width;
        dy_p4 = - half_width;
    }
    else
    {
        vec3 offset = vec3(miter_b * length_b, 0.0f);
        float para_offset = dot(- miter_b * length_b, v1);
        //float para_offset = dot(offset, v0);
        p3 = vec3(u_P * vec4(end_in_view.xyz + offset, 1.0f));
        p4 = vec3(u_P * vec4(end_in_view.xyz - offset, 1.0f));
        para_p3 = paraEnd - para_offset;
        para_p4 = paraEnd + para_offset;
        dy_p3 = half_width;
        dy_p4 = - half_width;
    }

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

void set_extreme_color(vec4 the_color, float radius)
{
    if(para_gshader < 0.0f)
    {
        set_side_color(the_color, para_gshader, dist_to_center_gshader, radius);
    }
    else if(para_gshader > total_length_gshader)
    {
        float delta = para_gshader - total_length_gshader;
        set_side_color(the_color, delta, dist_to_center_gshader, radius);
    }
    else
    {
        color = the_color;
    }
}

// the same as line.shader
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

    // no dash
    if(u_dashes_size == 0)
    {
        set_extreme_color(the_color, radius);
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
            if(total_length_gshader < u_dash_sum_length)
            {
                if(abs(para_gshader) < radius)
                {
                    set_side_color(the_color, abs(para_gshader),dy, radius);
                }
                else if(abs(para_gshader - total_length_gshader) < radius)
                {
                    set_side_color(the_color, abs(para_gshader - total_length_gshader), dy, radius);
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
            if(total_length_gshader < u_dash_sum_length)
            {
                set_extreme_color(the_color, radius);
                color_set = true;
            }
        }

        if(!color_set)
        {
            float factor = total_length_gshader / u_dash_sum_length;
            float factor_fract = fract(factor);
            float factor_num = fract(factor);
            float para_offset1 = factor_fract * 0.5f * u_dash_sum_length;   // parameter between (para_offset1, para_offset2) display line style, others display solid line or space
            float para_offset2 = total_length_gshader - para_offset1;

            // begin and end outside, display semicicle
            if(para < 0.0f || para > total_length_gshader)
            {
                set_extreme_color(the_color, radius);
            }
            // begin segment, display solid line or space
            else if(para >= 0.0f && para <= para_offset1)
            {
                if(!is_dot)
                {
                    color = the_color;
                }
                else
                {
                    if(para < para_offset1 - para)
                    {
                        set_side_color(the_color, para, dy, radius);
                    }
                    else
                    {
                        set_side_color(the_color, para_offset1 - para, dy, radius);
                    }
                }
            }
            // end segment, display solid line or space
            else if(para >= para_offset2 && para <= total_length_gshader)
            {
                if(!is_dot)
                {
                    color = the_color;
                }
                else
                {
                    if(para - para_offset2 < total_length_gshader - para)
                    {
                        set_side_color(the_color, para - para_offset2, dy, radius);
                    }
                    else
                    {
                        set_side_color(the_color, total_length_gshader - para, dy, radius);
                    }
                }
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
                bool is_zero = set_zero_point(the_color, para_display_round, dy, radius);
                // is dash line
                if(!is_zero)
                {
                    set_line_blank(the_color, para_display_round, dy, radius);
                }
            }
        }
    }
}
