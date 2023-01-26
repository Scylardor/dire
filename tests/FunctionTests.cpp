#include <catch2/catch_test_macros.hpp>

#include "catch2/matchers/catch_matchers.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include "Dire/Dire.h"

#include <map>
#include <sstream>

class noncopyable
{
public:
	noncopyable() = default;
	noncopyable(const noncopyable &) = delete;
	noncopyable& operator=(const noncopyable &) = delete;
};

class noncopyable_int : public noncopyable
{
	const int theAnswer = 42;
};

class emptyObject
{};

struct Test
{
	DIRE_DECLARE_TYPEINFO()

	DIRE_FUNCTION(int, namedParamTest, (int aTest))

	DIRE_FUNCTION(int, anonymousParamTest, bool);

	DIRE_FUNCTION(int, noArguments);

	DIRE_FUNCTION(short, multipleArguments, float bibi, int toto);

	DIRE_FUNCTION(void, templateArguments1, (std::vector<float>)&, int toto);
	DIRE_FUNCTION(void, templateArguments2, (std::map<int, bool>)&, int toto);

	void separateDeclarationTest(float & floatRef);
	DIRE_FUNCTION_TYPEINFO(separateDeclarationTest);

	DIRE_FUNCTION(void, refParam, int&);

	DIRE_FUNCTION(void, noncopyableRefParam, noncopyable&);

	DIRE_FUNCTION(void, passArrayByValue, std::vector<bool>);

	DIRE_FUNCTION(void, passObjectByValue, emptyObject);
};


void Test::refParam(int& test)
{
	test = 42;
}

void Test::noncopyableRefParam(noncopyable& test)
{
	(void)test;
}

void Test::passArrayByValue(std::vector<bool> /*unused*/)
{
}

void Test::passObjectByValue(emptyObject /*unused*/)
{
}

int Test::noArguments()
{
	return 42;
}

int Test::namedParamTest(int /*aTest*/)
{
	return 0;
}

int Test::anonymousParamTest(bool)
{
	return 0;
}

void Test::separateDeclarationTest(float& floatRef)
{
	floatRef = 1.f;
}

short Test::multipleArguments(float bibi, int toto)
{
	return (short)(bibi + (float) toto);
}

void Test::templateArguments1(std::vector<float>& floats, int toto)
{
	floats.push_back((float)toto);
}

void Test::templateArguments2(std::map<int, bool>& ints, int toto)
{
	ints[toto] = true;
}



TEST_CASE("Invoke Functions", "[Functions]")
{
	Test aTest;

	float result = 3.f;
	dire::Invoke("separateDeclarationTest", aTest, result);
	REQUIRE_THAT(result, Catch::Matchers::WithinRel(1.f));

	int resulti = dire::Invoke<int>("noArguments", aTest);
	REQUIRE(resulti == 42);

	short sho = dire::Invoke<short>("multipleArguments", aTest, 4.f, 3);
	REQUIRE(sho == 7);

	std::vector<float> floats;
	dire::Invoke("templateArguments1", aTest, floats, 2);
	REQUIRE(floats.size() == 1);
	REQUIRE_THAT(floats[0], Catch::Matchers::WithinRel(2.f));

	resulti = 0;
	dire::Invoke("refParam", aTest, resulti);
	REQUIRE(resulti == 42);

	// ensure there's no extraneous copy attempted
	// if params were not perfectly forwarded, it wouldn't compile.
	noncopyable_int nocopy;
	dire::Invoke("noncopyableRefParam", aTest, nocopy);
}

TEST_CASE("Function Type Info", "[Functions]")
{
	std::ostringstream ostr;

	for (const dire::FunctionInfo& funcInfo : Test::GetTypeInfo().GetFunctionList())
	{
		ostr << funcInfo.GetName() << ' ';
		ostr << "Type: " << funcInfo.GetReturnType().GetString() << ' ';
		ostr << "Parameters:";
		for (const dire::MetaType paramType : funcInfo.GetParametersTypes())
		{
			ostr << ' ' << paramType.GetString();
		}
		ostr << '\n';
	}
	auto result = ostr.str();
	REQUIRE(result == "namedParamTest Type: Int Parameters: Int\n\
anonymousParamTest Type: Int Parameters: Bool\n\
noArguments Type: Int Parameters:\n\
multipleArguments Type: Short Parameters: Float Int\n\
templateArguments1 Type: Void Parameters: Reference Int\n\
templateArguments2 Type: Void Parameters: Reference Int\n\
separateDeclarationTest Type: Void Parameters: Reference\n\
refParam Type: Void Parameters: Reference\n\
noncopyableRefParam Type: Void Parameters: Reference\n\
passArrayByValue Type: Void Parameters: Array\n\
passObjectByValue Type: Void Parameters: Object\n");
}

TEST_CASE("Reflectable Function Invoke Interface", "[Functions]")
{
	// TODO
	//aTest.TypedInvokeFunction<void, float>("zdzdzdz", 3.f);
	//funcInfo->TypedInvokeWithArgs<void>(&aTest, 3.f);
}
