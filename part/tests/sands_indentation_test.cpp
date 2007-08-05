// C++ S&S Indentation test.
// Copy this into kate and check that it indents corrently.
// Obviously that will be different for the different indentors, but this tries
// to check cases that I might not have thought of.
// TODO: Automate testing once the indentation actually works!


namespace Test
{

enum
{
	one,
	two,
	three,
	four
};

bool function(int a, bool b,
              int c, double d)
{
	for (int i = 0; i < b ? a : c; ++i)
		cout << "Number: " << i << endl;

	switch (classType)
	{
	case '1':
	case '/':
	case ';':
		break;
	case ':':
	colon:
		break;

	case Test::one:
		goto colon;
	}
	return Test::two;
}

}

class QWidget : public QObject
{
	Q_OBJECT
public:
	QWidget() : parent(null)
	{
	}
private:
	struct Car : Vehicle
	{

	};

private slots:
	Car test();
}

Car QWidget::test()
{
	// A comment.
	cerr << "this"
         << "is" << "a" << "test";
	/* A block
	comment */
}
