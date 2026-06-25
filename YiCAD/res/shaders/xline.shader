#shader vertex
#version 430
#define VERTEX 
#shader include "common.inl"

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 dir;

out vec3 dir_vshader;

void main()
{
    gl_Position = u_MV * vec4(pos, 1.0f);	// no project
    dir_vshader = normalize(dir);
}

// ---------------------------------
#shader geometry
#version 430
#define GEOMETRY 
#shader include "common.inl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 dir_vshader[];

out float para_gshader;
out float dist_to_center_gshader;

void emitVertices(float para, float para_v)
{
    para_gshader = para;
    dist_to_center_gshader = para_v;
}

void main()
{
    vec4 start_in_view = gl_in[0].gl_Position;
    vec3 dir = normalize(dir_vshader[0]);
    mat4 rMat=buildRotateZ(PI/2.0);
	vec4 normal = rMat*vec4(dir,1.0f);	

    float size_per_pixel = get_size_per_pixel();
	float width_pixel = get_line_width_pixel();
    float half_width_pixel = width_pixel / 2.0f;
    float half_width = size_per_pixel * half_width_pixel;
    float larger_width = max(u_viewportSize.x, u_viewportSize.y);
    float len = larger_width * 1.5f * size_per_pixel; 
    vec3 offset = vec3(normal.xy * half_width, 0.0f);
    vec3 p1, p2, p3, p4;
    float para_p1, para_p2, para_p3, para_p4;
    float dy_p1, dy_p2, dy_p3, dy_p4;

    // calculate the center screen projection to ray
    vec3 v_op = -start_in_view.xyz; 
    float proj_len = dot(v_op, dir);
    vec3 proj_pt = start_in_view.xyz + dir * proj_len;
    vec3 start_pt = proj_pt - dir * len;
    vec3 end_pt = proj_pt + dir * len;

    vec3 para_offset_vec = start_pt - start_in_view.xyz;
    float start_para = dot(para_offset_vec, dir);  // start parameter
    if(start_para < 0.0f)
    {
        start_para = mod(start_para, u_dash_sum_length);
        start_para += u_dash_sum_length;
    }

    para_p1 = start_para;
	para_p2 = start_para;
    float len3 = length(start_pt - end_pt);
    para_p3 = para_p1 + len3;
	para_p4 = para_p2 + len3;

	p1 = vec3(u_P * vec4(start_pt + offset, 1.0f));
	p2 = vec3(u_P * vec4(start_pt - offset, 1.0f));
	dy_p1 = half_width;
	dy_p2 = - half_width;

	p3 = vec3(u_P * vec4(end_pt + offset, 1.0f));
	p4 = vec3(u_P * vec4(end_pt - offset, 1.0f));
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
        color = the_color;
    }
    // with dash
    else
    {
        // one pixel opcupy noe than dash length, display no dash 
        if(size_per_pixel > u_dash_sum_length)
        {
            color = the_color;
        }
        else
        {
            float para_display_round = mod(para, u_dash_sum_length);
            if(para_display_round < 0.0f)
            {
                para_display_round += u_dash_sum_length;
            }
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