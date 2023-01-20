#include "catch2/catch_test_macros.hpp"

#include "TestClasses.h"

#include "dire/DireSubclass.h"

// Test for instantiation with and without automatic default constructor registration

dire_reflectable(struct DefaultInstantiated)
{
	DIRE_REFLECTABLE_INFO()
};

#define TMP_DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE
#undef DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE
#define DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE 0

dire_reflectable(struct CustomInstantiated)
{
	DIRE_REFLECTABLE_INFO()

	CustomInstantiated(int pConfig) :
		aConfigVariable(pConfig)
	{}

	DECLARE_INSTANTIATOR(int)

	int aConfigVariable = 0;
};

#undef DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE
#define DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE TMP_DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE
#undef TMP_DIRE_DEFAULT_CONSTRUCTOR_INSTANTIATE


TEST_CASE("Subclass Instantiate", "[Reflectable]")
{
	// generic instantiate
	dire::Subclass<a> aSubClass;
	REQUIRE(aSubClass.GetClassID() == a::GetTypeInfo().GetID());

	aSubClass.SetClass(c::GetTypeInfo().GetID());
	REQUIRE(aSubClass.GetClassID() == c::GetTypeInfo().GetID());
	a* newA = aSubClass.Instantiate();
	REQUIRE((nullptr != newA && dynamic_cast<c*>(newA) != nullptr));
	delete newA; // So far, Instantiate is handled with new, so deleting the allocated pointer is up to the caller.

	// explicit typed instantiate
	c* anotherInstance = aSubClass.Instantiate<c>(); // TODO: try that with noncopyable...
	REQUIRE(nullptr != anotherInstance);
	delete anotherInstance;

	// change of class ID
	aSubClass.SetClass(b::GetTypeInfo().GetID());
	REQUIRE(aSubClass.GetClassID() == b::GetTypeInfo().GetID());
	a* aB = aSubClass.Instantiate();
	REQUIRE((nullptr != aB && dynamic_cast<b*>(aB) != nullptr));
	delete aB;

	// with a custom instantiator
	dire::Subclass<CustomInstantiated> customSubClass;
	CustomInstantiated* custom = customSubClass.Instantiate(42);
	REQUIRE((nullptr != custom && custom->aConfigVariable == 42));
	delete custom;

	// invalid ID
	aSubClass.SetClass(dire::INVALID_REFLECTABLE_ID);
	REQUIRE(aSubClass.GetClassID() == dire::INVALID_REFLECTABLE_ID);
	a* null = aSubClass.Instantiate();
	REQUIRE(null == nullptr);

	// valid id but not a base of the subclass
	aSubClass.SetClass(SuperCompound::GetTypeInfo().GetID());
	REQUIRE(aSubClass.GetClassID() == SuperCompound::GetTypeInfo().GetID());
	null = aSubClass.Instantiate();
	REQUIRE(null == nullptr);
}

TEST_CASE("Clone", "[Reflectable]")
{
	testcompound2 clonedComp;
	clonedComp.leet = 123456789;
	clonedComp.copyable.aUselessProp = 42.f;
	testcompound2* clone = clonedComp.Clone<testcompound2>();
	REQUIRE((clone && clone->leet == clonedComp.leet && clone->copyable.aUselessProp == clonedComp.copyable.aUselessProp && &clone->leet != &clonedComp.leet));
	delete clone; // So far, Cloning is handled with new, so deleting the pointer is up to the caller.
}

TEST_CASE("IsA", "[Reflectable]")
{
	a anA;
	b aB;
	c aC;
	SuperCompound notPartOfTheFamily;

	REQUIRE((anA.IsA<a>() && !anA.IsA<b>() && !anA.IsA<c>() && !anA.IsA<SuperCompound>()));
	REQUIRE((aB.IsA<a>() && aB.IsA<b>() && !aB.IsA<c>() && !aB.IsA<SuperCompound>()));
	REQUIRE((aC.IsA<a>() && aC.IsA<b>() && aC.IsA<c>() && !aC.IsA<SuperCompound>()));
	REQUIRE((!notPartOfTheFamily.IsA<a>() && !notPartOfTheFamily.IsA<b>() && !notPartOfTheFamily.IsA<c>() && notPartOfTheFamily.IsA<SuperCompound>()));
}

TEST_CASE("TypeInfo Name", "[Reflectable]")
{
	REQUIRE(a::GetTypeInfo().GetName() == "a");
	REQUIRE(b::GetTypeInfo().GetName() == "b");
	REQUIRE(c::GetTypeInfo().GetName() == "c");
	REQUIRE(SuperCompound::GetTypeInfo().GetName() == "SuperCompound");
	REQUIRE(testNS::Nested::GetTypeInfo().GetName() == "testNS::Nested");
	REQUIRE(testNS::Nested2::GetTypeInfo().GetName() == "testNS::Nested2");
}

TEST_CASE("Reflectable Instantiate", "[Reflectable]")
{
	// use default constructor as instantiator
	DefaultInstantiated* deft = dire::TypeInfoDatabase::GetSingleton().InstantiateClass<DefaultInstantiated>();
	REQUIRE(deft != nullptr);

	// use custom constructor as instantiator
	CustomInstantiated* custom = dire::TypeInfoDatabase::GetSingleton().InstantiateClass<CustomInstantiated>(1337);
	REQUIRE((custom != nullptr && custom->aConfigVariable == 1337));

	// no constructor takes those parameters, should fail
	DefaultInstantiated* nodeft = dire::TypeInfoDatabase::GetSingleton().InstantiateClass<DefaultInstantiated>(1337);
	CustomInstantiated* nocustom = dire::TypeInfoDatabase::GetSingleton().InstantiateClass<CustomInstantiated>();
	REQUIRE((nodeft == nullptr && nocustom == nullptr));

	delete deft;
	delete custom;
}