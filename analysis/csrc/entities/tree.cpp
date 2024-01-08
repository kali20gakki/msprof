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

#include "analysis/csrc/entities/tree.h"

#include <queue>
#include <string>

#include "analysis/csrc/utils/utils.h"

namespace Analysis {
namespace Entities {

namespace {
// EventType类型对应字符串的映射关系
// EventType和EventTypeString一一对应
std::vector<std::string> EventTypeString{
    "Api",
    "Event",
    "NodeBasicInfo",
    "TensorInfo",
    "HcclInfo",
    "ContextId",
    "GraphIdMap",
    "FusionOpInfo",
    "TaskTrack",
    "MemoryCopy",
    "Dummy",
    "Invalid"
};
}

std::shared_ptr<TreeNode> Tree::GetRoot() const
{
    return root_;
}

std::string Tree::GetTreeLevelStr(const std::shared_ptr<TreeNode> &node) const
{
    std::string lstr;
    for (const auto &r: node->records) {
        lstr += "[" + EventTypeString[static_cast<unsigned long>(r->info.type)] +
            Utils::ConvertToString(r->info.start, r->info.end) + "] ";
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
        auto levelSize = q.size();
        std::string lstr{};
        for (size_t i = 0; i < levelSize; ++i) {
            std::shared_ptr<TreeNode> node = q.front();
            q.pop();
            lstr += EventTypeString[static_cast<unsigned long>(
                node->event->info.type)] + Utils::ConvertToString(node->event->info.start,
                                                                  node->event->info.end) + " ";
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