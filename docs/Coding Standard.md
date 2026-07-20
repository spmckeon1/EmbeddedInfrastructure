# EmbeddedInfrastructure Coding Standard

Classes shall be organized in this order:

## Class Layout

## For classes with protected members:

class ClassName {
private:
    // Private data members

    // Private helper functions

protected:
    // Protected members (used rarely)

public:
    // Constructors/destructor

    // Public interface
};

### For classes without protected members:

class Logging {
private:
private:
    // Private data members

    // Private helper functions


public:
    // Constructors/destructor

    // Public interface
};