#include <map>
#include <string>
#include <vector>

#include "Dire/DireProperty.h"
#include "dire/DireReflectable.h"
#include "dire/Utils/DireTypeTraits.h"
#include <unordered_map>

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

static_assert(dire::HasArraySemantics_v<int[10]>);
static_assert(dire::HasArraySemantics_v<std::vector<int>>);
static_assert(dire::HasArraySemantics_v<std::string>);

static_assert(dire::HasMapSemantics_v<std::map<int, bool>>);
static_assert(dire::HasMapSemantics_v<std::unordered_map<int, bool>>);

static_assert(!dire::HasMapSemantics_v<int[10]>);
static_assert(!dire::HasMapSemantics_v<std::vector<int>>);

static_assert(!dire::HasArraySemantics_v<std::map<int, bool>>);
static_assert(!dire::HasArraySemantics_v<std::unordered_map<int, bool>>);


dire_reflectable(struct Compound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(int, compint, 0x2a2a)
};

static_assert(!dire::HasArraySemantics_v<Compound>);
static_assert(!dire::HasMapSemantics_v<Compound>);
