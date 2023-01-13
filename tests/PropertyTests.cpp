
#include <catch2/catch_test_macros.hpp>
#include "Dire/Dire.h"
#include "dire/DireProperty.inl"
#include "dire/DireReflectable.h"

// Declare some structs to be able to create a hierarchy of reflectable types...

dire_reflectable(struct SuperCompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_ARRAY_PROPERTY(int, titi, [5])

};


dire_reflectable(struct Copyable)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(float, aUselessProp, 4.f)

	Copyable() = default;
	Copyable(Copyable const& pOther)
	{
		aUselessProp = pOther.aUselessProp;
	}

	Copyable& operator=(Copyable const& pOther)
	{
		aUselessProp = pOther.aUselessProp;
		return *this;
	}
};

dire_reflectable(struct testcompound2)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(int, leet, 1337);
	DIRE_PROPERTY(Copyable, copyable);

	testcompound2() = default;
};

dire_reflectable(struct testcompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(int, compint, 0x2a2a)
	DIRE_PROPERTY(testcompound2, compleet)

	testcompound() = default;
};


dire_reflectable(struct MegaCompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(int, compint, 0x2a2a)
	DIRE_PROPERTY(testcompound2, compleet)
	DIRE_ARRAY_PROPERTY(SuperCompound, toto, [3])

	MegaCompound() = default;
};

dire_reflectable(struct UltraCompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(MegaCompound, mega)

	UltraCompound() = default;
};

dire_reflectable(struct a)
{
	DIRE_REFLECTABLE_INFO()

	int test()
	{
		printf("I'm just a test!\n");
		return 42;
	}

	DIRE_FUNCTION_TYPEINFO(test);

	virtual ~a() = default;

	virtual void print()
	{
		printf("a\n");
	}

	DIRE_PROPERTY(bool, atiti, 0)
	DIRE_PROPERTY(float, atoto, 0)
};


struct NotAReflectable
{
	bool bibi;
	float fat[4];
};

dire_reflectable(struct b, a, NotAReflectable) // an example of multiple inheritance
{
	DIRE_REFLECTABLE_INFO()
	void Roger()
	{
		printf("Roger that sir\n");
	}
	DIRE_FUNCTION_TYPEINFO(Roger);

	void stream(int) {}

	void print() override
	{
		printf("yolo\n");
	}

	DIRE_PROPERTY(double, bdouble, 0)

	DIRE_PROPERTY(testcompound, compvar)
	char bpouiet[10];
};

dire_reflectable(struct c, b)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(unsigned, ctoto, 0xDEADBEEF)

	DIRE_PROPERTY((std::vector<int>), aVector, std::initializer_list<int>{1, 2, 3})
	DIRE_ARRAY_PROPERTY(int, anArray, [10])
	DIRE_ARRAY_PROPERTY(int, aMultiArray, [10][10])

	DIRE_PROPERTY(MegaCompound, mega);
	DIRE_PROPERTY(UltraCompound, ultra);

	c() = default;

	c(int, bool)
	{
		printf("bonjour int bool\n");
	}

	void print() override
	{
		printf("c\n");
	}
};


TEST_CASE("GetProperty Simple", "[Property]")
{
	c superC;

	// direct property
	unsigned const* compint = superC.GetProperty<unsigned>("ctoto");
	assert(*compint == 0xdeadbeef);


	// inherited property

	// in compound

	// in array

	// in multiarray

	// in nested compound

	// nested arrays in compounds

	// non existing
}


TEST_CASE("GetProperty Array", "[Property]")
{
	c superC;

	// direct property

	// inherited

	// in multiarray

	// in nested compound

	// nested arrays in compounds

}


TEST_CASE("GetProperty Map", "[Property]")
{
	c superC;

	// direct property

	// inherited

	// in nested compound

	// nested map in compounds


}




TEST_CASE("SetProperty", "[Property]")
{

}