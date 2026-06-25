/*
 * Copyright (C) 2024-2026 YiCAD Contributors
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/// @file GLPenData.h
/// @brief OpenGL画笔数据（线型、颜色、线宽）定义

#ifndef GLPENDATA_H
#define GLPENDATA_H

#include <vector>
#include <unordered_map>

namespace opengl
{

/// @brief 线型、颜色、线宽构造体
struct GLPenData
{
    // dash
    std::vector<float>              m_dashes_data;
    int                             m_dashes_size = 0;

    // dash的其他参数（m_dashes_data计算得到）
    float sum_length = 0.0f;                        ///< 总长
    std::vector<float> zero_paras;                   ///< dash中点位置
    std::vector<float> para_pairs;                   ///< dash中非空白段
    std::vector<float> blank_para_pairs;             ///< dash中空白段
    int pair_num = 0;                                ///< dash中非空白段数（para_pairs.size()的1/2）
    int blank_pair_num = 0;                          ///< dash中空白段数（blank_para_pairs.size()的1/2）
    int zero_num = 0;                                ///< dash中点个数（zero_paras.size()）

    // 颜色
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    float alpha = 0.0f;

    // 线宽
    float lineWidth = 0.0f;

    /// @brief 设置线型并计算一些参数
    /// @param [in] dashes 线型数据数组
    /// @param [in] num_dashes 线型数据元素个数
    void setDash(const double* dashes, const int num_dashes)
    {
        constexpr float TOL = 1.0e-8f;              ///< 零值容差

        std::vector<float> data;
        data.reserve(num_dashes);
        for (int k = 0; k < num_dashes; k++)
        {
            data.emplace_back(dashes[k]);
        }
        m_dashes_data = data;
        m_dashes_size = num_dashes;
        para_pairs.clear();
        zero_paras.clear();

        // 计算一些参数
        float sum_length_temp = 0.0f;	//总长
        int i = 0;
        for (; i < m_dashes_size; i++)
        {
            float val = m_dashes_data[i];
            if (val > TOL)
            {
                para_pairs.emplace_back(sum_length_temp);
                para_pairs.emplace_back(sum_length_temp + val);
            }
            else if (val < -TOL)
            {
                blank_para_pairs.emplace_back(sum_length_temp);
                blank_para_pairs.emplace_back(sum_length_temp + std::abs(val));
            }

            sum_length_temp += std::abs(val);
            if (std::abs(val) <= TOL)
            {
                zero_paras.emplace_back(sum_length_temp);
            }
        }
        this->sum_length = sum_length_temp;
        pair_num = static_cast<int>(para_pairs.size()) / 2;
        blank_pair_num = static_cast<int>(blank_para_pairs.size()) / 2;
        zero_num = static_cast<int>(zero_paras.size());
    }
};

using GLPenDataMap = std::unordered_map<int, GLPenData>; ///< key为画笔的id

}

#endif //GLPENDATA_H
