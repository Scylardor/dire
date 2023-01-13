#include "catch2/catch_test_macros.hpp"

#include "TestClasses.h"

#include "dire/DireSubclass.h"

TEST_CASE("Subclass Instantiate", "[Reflectable]")
{
	// generic instantiate
	dire::Subclass<a> aSubClass;
	REQUIRE(aSubClass.GetClassID() == a::GetClassReflectableTypeInfo().GetID());

	aSubClass.SetClass(c::GetClassReflectableTypeInfo().GetID());
	REQUIRE(aSubClass.GetClassID() == c::GetClassReflectableTypeInfo().GetID());
	a* newA = aSubClass.Instantiate();
	REQUIRE((nullptr != newA && dynamic_cast<c*>(newA) != nullptr));
	delete newA; // So far, Instantiate is handled with new, so deleting the allocated pointer is up to the caller.

	// explicit typed instantiate
	c* anotherInstance = aSubClass.Instantiate<c>(42, true); // TODO: try that with noncopyable...
	REQUIRE(nullptr != anotherInstance);
	delete anotherInstance;

	// change of class ID
	aSubClass.SetClass(b::GetClassReflectableTypeInfo().GetID());
	REQUIRE(aSubClass.GetClassID() == b::GetClassReflectableTypeInfo().GetID());
	a* aB = aSubClass.Instantiate();
	REQUIRE((nullptr != aB && dynamic_cast<b*>(aB) != nullptr));
	delete aB;

	// invalid ID
	aSubClass.SetClass((unsigned) -1); // TODO: reflectableID
	REQUIRE(aSubClass.GetClassID() == (unsigned)-1);
	a* null = aSubClass.Instantiate();
	REQUIRE(null == nullptr);

	// valid id but not a base of the subclass
	aSubClass.SetClass(SuperCompound::GetClassReflectableTypeInfo().GetID());
	REQUIRE(aSubClass.GetClassID() == SuperCompound::GetClassReflectableTypeInfo().GetID());
	null = aSubClass.Instantiate();
	REQUIRE(null == nullptr);
}

TEST_CASE("Clone", "[Reflectable]")
{
	testcompound2 clonedComp;
	clonedComp.leet = 123456789;
	clonedComp.copyable.aUselessProp = 42.f;
	testcompound2* clone = clonedComp.Clone<testcompound2>();
	REQUIRE((clone->leet == clonedComp.leet && clone->copyable.aUselessProp == clonedComp.copyable.aUselessProp && &clone->leet != &clonedComp.leet));
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