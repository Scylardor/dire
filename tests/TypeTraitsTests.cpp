#include <string>
#include <vector>

#include "dire/DireTypeTraits.h"

static_assert(dire::has_ArrayBrackets_v<std::string>);
static_assert(dire::has_ArrayBrackets_v<std::vector<int>>);
static_assert(!dire::has_ArrayBrackets_v<int[]>);

static_assert(dire::has_insert_v<std::string>);
static_assert(dire::has_insert_v<std::vector<int>>);
static_assert(!dire::has_insert_v<int[]>);

static_assert(dire::has_erase_v<std::string>);
static_assert(dire::has_erase_v<std::vector<int>>);
static_assert(!dire::has_erase_v<int[]>);

static_assert(dire::has_clear_v<std::string>);
static_assert(dire::has_clear_v<std::vector<int>>);
static_assert(!dire::has_clear_v<int[]>);

static_assert(dire::has_size_v<std::string>);
static_assert(dire::has_size_v<std::vector<int>>);
static_assert(!dire::has_size_v<int[]>);