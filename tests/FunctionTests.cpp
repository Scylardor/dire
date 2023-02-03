#include <catch2/catch_test_macros.hpp>

#include "catch2/matchers/catch_matchers.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include "dire/Dire.h"

#include "TestClasses.h"

#include <map>
#include <sstream>



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

void Test::nothing()
{
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
	REQUIRE(nocopy.theAnswer == 42);
	dire::Invoke("noncopyableRefParam", aTest, nocopy);
	REQUIRE(nocopy.theAnswer == 42);
}

TEST_CASE("Function Type Info", "[Functions]")
{
	std::ostringstream ostr;

	for (const dire::FunctionInfo& funcInfo : Test::GetTypeInfo().GetFunctionList())
	{
		ostr << funcInfo.GetName() << ' ';
		ostr << "Type: " << funcInfo.GetReturnType().GetString() << ' ';
		ostr << "Parameters:";
		for (const dire::MetaType& paramType : funcInfo.GetParametersTypes())
		{
			ostr << ' ' << paramType.GetString();
		}
		ostr << '\n';
	}
	auto result = ostr.str();
	REQUIRE(result ==  "namedParamTest Type: Int Parameters: Int\n\
anonymousParamTest Type: Int Parameters: Bool\n\
noArguments Type: Int Parameters:\n\
nothing Type: Void Parameters:\n\
multipleArguments Type: Short Parameters: Float Int\n\
templateArguments1 Type: Void Parameters: Reference Int\n\
templateArguments2 Type: Void Parameters: Reference Int\n\
separateDeclarationTest Type: Void Parameters: Reference\n\
refParam Type: Void Parameters: Reference\n\
noncopyableRefParam Type: Void Parameters: Reference\n\
passArrayByValue Type: Void Parameters: Array\n\
passObjectByValue Type: Void Parameters: Object\n\
");
}

TEST_CASE("Reflectable Function Invoke Interface", "[Functions]")
{
	Test aTest;

	// bad name (does nothing)
	const dire::FunctionInfo* funcInfo = aTest.GetFunction("zdzdzdz");
	REQUIRE(funcInfo == nullptr);
	aTest.TypedInvokeFunction<void>("zdzdzdz", 3.f);

	// no arguments
	int result = aTest.TypedInvokeFunction<int>("noArguments");
	REQUIRE(result == 42);

	// arguments, void return
	result = 1337;
	aTest.TypedInvokeFunction<void>("refParam", result);
	REQUIRE(result == 42);

	// no arguments, void return
	aTest.TypedInvokeFunction<void>("nothing");

	// multiple arguments
	short sres = aTest.TypedInvokeFunction<short>("multipleArguments", 3.f, 1);
	REQUIRE(sres == 4);


}
