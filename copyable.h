#ifndef MUODUO_BASE_COPYABLE_H
#define MUODUO_BASE_COPYABLE_H

namespace muduo
{
    // 标识类：A tag class emphasises the objects are copyable.
    // 空基类：The empty base class optimization applies.
    // 值类型：Any derived class of copyable should be a value type.
    class copyable
    {
    };
    
    
};

#endif // MUODUO_BASE_COPYABLE_H