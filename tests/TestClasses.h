#pragma once
#include "Dire/DireProperty.h"
#include "dire/DireReflectable.h"

#include <map>

// Declare some structs to be able to create a hierarchy of reflectable types...

namespace testNS
{

	dire_reflectable(struct Nested)
	{
		DIRE_REFLECTABLE_INFO()
		DIRE_PROPERTY(bool, useless, true)
	};

	dire_reflectable(struct Nested2)
	{
		DIRE_REFLECTABLE_INFO()

		DIRE_PROPERTY(bool, useless, true)

		DIRE_ARRAY_PROPERTY(char, pouet, [10])
	};

	dire_reflectable(struct Nested3, Nested2)
	{
		DIRE_REFLECTABLE_INFO()

		DIRE_PROPERTY(bool, allo, true)
	};
}

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

	int pouet = 1;

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

	bool operator==(const MegaCompound& other)
	{
		return compint == other.compint && compleet.leet == other.compleet.leet && compleet.copyable.aUselessProp == other.compleet.copyable.aUselessProp
			&& memcmp((const void*) & toto, (const void*) & other.toto, sizeof(toto)) == 0;
	}
};

dire_reflectable(struct UltraCompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(MegaCompound, mega)

	UltraCompound() = default;

	bool operator==(const UltraCompound & other)
	{
		return mega == other.mega;
	}
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
	}


};

dire_reflectable(struct mapType)
{
	DIRE_REFLECTABLE_INFO()
	DIRE_PROPERTY((std::map<int, bool>), aEvenOddMap);
};

dire_reflectable(struct MapCompound)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY((std::map<int, bool>), aBoolMap);

	DIRE_PROPERTY((std::map<int, SuperCompound>), aSuperMap);
};

dire_reflectable(struct d, c)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY((std::map<int, bool>), aMap);

	DIRE_PROPERTY(int, xp, 42)
	DIRE_PROPERTY((std::map<int, bool>), aBoolMap);
	DIRE_PROPERTY((std::map<int, testcompound2>), aFatMap);
	DIRE_PROPERTY((std::map<int, std::map<bool, int>>), aMapInMap);

	DIRE_PROPERTY(MapCompound, aStruct);
};

DIRE_SEQUENTIAL_ENUM(Kings, int, Philippe, Alexandre, Cesar, Charles);
DIRE_BITMASK_ENUM(BitEnum, int, one, two, four, eight);
DIRE_BITMASK_ENUM(Jacks, short, Ogier, Lahire, Hector, Lancelot);
DIRE_BITMASK_ENUM(Queens, short, Judith, Rachel, Pallas, Argine);

enum class Faces : uint8_t
{
	Jack,
	Queen,
	King
};

dire_reflectable(struct enumTestType)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY(Faces, aTestFace);
	DIRE_PROPERTY(Kings, bestKing, Kings::Alexandre);
	DIRE_ARRAY_PROPERTY(Kings, worstKings, [2])
	DIRE_PROPERTY((std::vector<Kings>), playableKings);
	DIRE_PROPERTY((std::map<Queens, bool>), allowedQueens);
	DIRE_PROPERTY((std::map<int, Jacks>), pointsPerJack);

	bool operator==(const enumTestType & pRhs) const
	{
		return aTestFace == pRhs.aTestFace && bestKing == pRhs.bestKing && memcmp(worstKings, pRhs.worstKings, sizeof(Kings) * 2) == 0
			&& playableKings == pRhs.playableKings && allowedQueens == pRhs.allowedQueens && pointsPerJack == pRhs.pointsPerJack;
	}
};


dire_reflectable(struct metadatas)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_PROPERTY((std::map<int, bool>), aMap);

	DIRE_PROPERTY(int, xp, DIRE_NS::Metadata<DIRE_NS::IValueRange<1, 10>>(), 42)

	DIRE_PROPERTY(bool, isTransient, DIRE_NS::Metadata<DIRE_NS::Transient>(), true)

	DIRE_PROPERTY(float, shouldBeIgnored, DIRE_NS::Metadata<DIRE_NS::NotSerializable>(), 4.f)

	DIRE_PROPERTY(enumTestType, shouldNeverBeIgnored, DIRE_NS::Metadata<DIRE_NS::Serializable>())

	DIRE_PROPERTY(int, multiMetadata, DIRE_NS::Metadata<DIRE_NS::IValueRange<1, 10>, DIRE_NS::Transient>())

#if DIRE_HAS_CPP20

	DIRE_PROPERTY(float, rangedFloat, DIRE_NS::Metadata <DIRE_NS::FValueRange<0.f, 1.f>>());

	DIRE_PROPERTY(std::string, customName, DIRE_NS::Metadata <DIRE_NS::DisplayName<"MyString">>(), "aString");

#endif
};

class noncopyable
{
public:
	noncopyable() = default;
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;
};

class noncopyable_int : public noncopyable
{
public:
	const int theAnswer = 42;

};

class emptyObject
{};


dire_reflectable(struct Test)
{
	DIRE_REFLECTABLE_INFO()

	DIRE_FUNCTION(int, namedParamTest, (int aTest))

	DIRE_FUNCTION(int, anonymousParamTest, bool);

	DIRE_FUNCTION(int, noArguments);

	DIRE_FUNCTION(void, nothing);

	DIRE_FUNCTION(short, multipleArguments, float bibi, int toto);

	DIRE_FUNCTION(void, templateArguments1, (std::vector<float>)&, int toto);
	DIRE_FUNCTION(void, templateArguments2, (std::map<int, bool>)&, int toto);

	void separateDeclarationTest(float& floatRef);
	DIRE_FUNCTION_TYPEINFO(separateDeclarationTest);

	DIRE_FUNCTION(void, refParam, int&);

	DIRE_FUNCTION(void, noncopyableRefParam, noncopyable&);

	DIRE_FUNCTION(void, passArrayByValue, std::vector<bool>);

	DIRE_FUNCTION(void, passObjectByValue, emptyObject);

};