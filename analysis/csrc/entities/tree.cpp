/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree.cpp
 * Description        : Tree模块：包括TreeNode数据结构和Tree
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */
#include <queue>
#include <string>
#include "tree.h"
namespace Analysis {
namespace Entities {

std::shared_ptr<TreeNode> Tree::GetRoot() const
{
    return root_;
}

std::string Tree::GetTreeLevelStr(std::shared_ptr <TreeNode> &node)
{
    std::string lstr;
    if (!node->records.empty()) {
        for (const auto &r: node->records) {
            lstr += "[" + r->desc + "] ";
        }
    }
    return lstr;
}

std::vector<std::string> Tree::Show()
{
    if (!root_) {
        return {};
    }
    std::vector<std::string> levelStr;
    std::queue<std::shared_ptr<TreeNode>> q;
    q.push(root_);
    while (!q.empty()) {
        auto level_size = q.size();
        std::string lstr{};
        for (int i = 0; i < level_size; ++i) {
            std::shared_ptr<TreeNode> node = q.front();
            q.pop();
            lstr += node->event->desc + " ";
            lstr += GetTreeLevelStr(node);
            for (const auto &child: node->children) {
                q.push(child);
            }
        }
        levelStr.emplace_back(lstr);
    }
    return levelStr;
}

} // namespace Entities
} // namespace Analysis