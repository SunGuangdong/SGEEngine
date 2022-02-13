#include "sge_utils/containers/Optional.h"
#include "doctest/doctest.h"

#include <string>
using namespace sge;

struct Tester {
	Tester() = delete;
	Tester(int& constr, int& destr)
	    : constr(&constr)
	    , destr(&destr)
	{
		if (this->constr)
			(*this->constr)++;
	}
	~Tester()
	{
		if (this->destr)
			(*this->destr)++;
	}
	Tester(const Tester& ref)
	{
		constr = ref.constr;
		destr = ref.destr;
		if (constr)
			(*constr)++;
		isCopyConstructed = true;
	}

	Tester(Tester&& ref)
	    : constr(ref.constr)
	    , destr(ref.destr)
	{
		this->isMoveConstructed = true;

		if (constr)
			(*constr)++;
		ref.constr = nullptr;
		ref.destr = nullptr;
		ref.isStolenWithMoveCtor = true;
	}

	Tester& operator=(Tester&& ref)
	{
		constr = ref.constr;
		destr = ref.destr;
		ref.constr = nullptr;
		ref.destr = nullptr;
		ref.isStolenWithMoveAssign = true;
		return *this;
	}

	Tester& operator=(const Tester& ref)
	{
		constr = ref.constr;
		destr = ref.destr;
		return *this;
	}

	int* constr = nullptr;
	int* destr = nullptr;

	bool isCopyConstructed = false;
	bool isMoveConstructed = false;

	bool isStolenWithMoveCtor = false;
	bool isStolenWithMoveAssign = false;
};

TEST_CASE("Optional Equal and not Equal")
{
	const char* const kTestCStr = "Was it a wonder to describe it!";

	Optional<std::string> x;

	CHECK_FALSE(x.isValid());
	x = kTestCStr;
	CHECK(x.isValid());
	CHECK(x.get() == kTestCStr);
	CHECK_FALSE(x.get() != kTestCStr);

	Optional<std::string> y = x;
	CHECK(x.isValid());
	CHECK(y.get() == kTestCStr);
	CHECK_FALSE(y.get() != kTestCStr);

	x.reset();
	CHECK_FALSE(x.isValid());
	y = x;
	CHECK_FALSE(y.isValid());
}

TEST_CASE("Optional Copy Constructor from T")
{
	int c = 0;
	int d = 0;

	{
		Tester tester{c, d};
		CHECK_FALSE(tester.isCopyConstructed);
		CHECK_FALSE(tester.isMoveConstructed);
		CHECK_FALSE(tester.isStolenWithMoveCtor);
		CHECK_FALSE(tester.isStolenWithMoveAssign);
		CHECK(c == 1);
		CHECK(d == 0);

		Optional<Tester> opt(tester);
		CHECK(opt.isValid());
		CHECK(opt->isCopyConstructed);
		CHECK_FALSE(opt->isMoveConstructed);
		CHECK_FALSE(opt->isStolenWithMoveCtor);
		CHECK_FALSE(opt->isStolenWithMoveAssign);
		CHECK(c == 2);
		CHECK(d == 0);

		opt.~Optional();
		CHECK(c == 2);
		CHECK(d == 1);
	}

	CHECK(c == 2);
	CHECK(d == 2);
}

TEST_CASE("Optional Copy Constructor from Optional")
{
	int c = 0;
	int d = 0;

	{
		Optional<Tester> optSrc(Tester{c, d});
		CHECK(optSrc->isMoveConstructed);
		CHECK(c == 2);
		CHECK(d == 0);

		Optional<Tester> opt(optSrc);
		CHECK(opt.isValid());
		CHECK(opt->isCopyConstructed);
		CHECK_FALSE(opt->isMoveConstructed);
		CHECK_FALSE(opt->isStolenWithMoveCtor);
		CHECK_FALSE(opt->isStolenWithMoveAssign);
		CHECK(c == 3);
		CHECK(d == 0);
	}

	CHECK(c == 3);
	CHECK(d == 2);
}

TEST_CASE("Optional Assign from T")
{
	int c = 0;
	int d = 0;

	{
		Optional<Tester> opt;
		CHECK_FALSE(opt.isValid());
		Tester tester{c, d};
		opt = tester;
		CHECK(opt.isValid());
		CHECK(c == 2);
		CHECK(d == 0);
	}

	CHECK(c == 2);
	CHECK(d == 2);
}

TEST_CASE("Optional Assign from Optional")
{
	int c = 0;
	int d = 0;

	{
		Optional<Tester> optSrc(Tester{c, d});
		CHECK(optSrc->isMoveConstructed);
		CHECK(c == 2);
		CHECK(d == 0);

		Optional<Tester> opt;
		CHECK_FALSE(opt.isValid());
		opt = optSrc;
		CHECK(opt.isValid());
		CHECK(c == 3);
		CHECK(d == 0);
	}

	CHECK(c == 3);
	CHECK(d == 2);
}

TEST_CASE("Optional Move Construct from T")
{
	int c = 0;
	int d = 0;

	{
		Tester tester{c, d};
		CHECK(c == 1);
		CHECK(d == 0);

		Optional<Tester> opt(std::move(tester));
		CHECK(tester.isStolenWithMoveCtor);

		CHECK(opt.isValid());
		CHECK_FALSE(opt->isCopyConstructed);
		CHECK(opt->isMoveConstructed);
		CHECK_FALSE(opt->isStolenWithMoveCtor);
		CHECK_FALSE(opt->isStolenWithMoveAssign);

		CHECK(c == 2);
		CHECK(d == 0);

		opt.~Optional();
		CHECK(c == 2);
		CHECK(d == 1);
	}

	CHECK(c == 2);
	CHECK(d == 1);
}

TEST_CASE("Optional Move Construct from Optional")
{
	int c = 0;
	int d = 0;

	{
		Optional<Tester> optSrc(Tester{c, d});
		CHECK(optSrc->isMoveConstructed);
		CHECK(c == 2);
		CHECK(d == 0);

		Optional<Tester> opt(std::move(optSrc));
		CHECK_FALSE(optSrc.isValid());

		CHECK(opt.isValid());
		CHECK_FALSE(opt->isCopyConstructed);
		CHECK(opt->isMoveConstructed);
		CHECK_FALSE(opt->isStolenWithMoveCtor);
		CHECK_FALSE(opt->isStolenWithMoveAssign);

		CHECK(c == 3);
		CHECK(d == 0);

		opt.~Optional();
		CHECK(c == 3);
		CHECK(d == 1);
	}

	CHECK(c == 3);
	CHECK(d == 1);
}

TEST_CASE("Optional Move Assign from T")
{
	int c = 0;
	int d = 0;

	{
		Tester tester{c, d};
		CHECK(c == 1);
		CHECK(d == 0);

		Optional<Tester> opt = tester;
		CHECK(c == 2);
		CHECK(d == 0);
		opt = std::move(tester);
		CHECK(tester.isStolenWithMoveAssign);

		CHECK(opt.isValid());
		CHECK(opt->isCopyConstructed);
		CHECK_FALSE(opt->isMoveConstructed);
		CHECK_FALSE(opt->isStolenWithMoveCtor);
		CHECK_FALSE(opt->isStolenWithMoveAssign);

		CHECK(c == 2);
		CHECK(d == 0);

		opt.~Optional();
		CHECK(c == 2);
		CHECK(d == 1);
	}

	CHECK(c == 2);
	CHECK(d == 1);
}

TEST_CASE("Optional Move Assign from Optional")
{
	int c = 0;
	int d = 0;

	{
		Optional<Tester> optSrc(Tester{c, d});
		CHECK(optSrc->isMoveConstructed);
		CHECK(c == 2);
		CHECK(d == 0);

		Optional<Tester> opt;
		opt = std::move(optSrc);
		CHECK(c == 3);
		CHECK_FALSE(optSrc.isValid());

		CHECK(opt.isValid());
		CHECK_FALSE(opt->isCopyConstructed);
		CHECK(opt->isMoveConstructed);
		CHECK_FALSE(opt->isStolenWithMoveCtor);
		CHECK_FALSE(opt->isStolenWithMoveAssign);

		CHECK(c == 3);
		CHECK(d == 0);

		opt.~Optional();
		CHECK(c == 3);
		CHECK(d == 1);
	}

	CHECK(c == 3);
	CHECK(d == 1);
}

TEST_CASE("Optional T&")
{
	int x;
	Optional<const int> v = x;
}
