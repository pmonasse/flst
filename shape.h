#ifndef SHAPE_H
#define SHAPE_H
#include <vector>

/// Structure for a pixel, 2 coordinates in image plane.
struct LsPoint {
    short int x;
    short int y;
};

/// Structure for a shape (connected component of level set with filled holes)
struct LsShape {
    typedef bool Type; // Better than enum for memory usage
    static const Type INF=false, SUP=true;

    Type type; ///< Inf or sup level set
    unsigned char gray; ///< Gray level of the level set
    bool bIgnore; ///< Should the shape be ignored?
    bool bBoundary; ///< Does the shape meets the border of the image?

    int area; ///< Number of pixels in the shape
    LsPoint* pixels; ///< Array of pixels in shape
#ifdef BOUNDARY
    std::vector<LsPoint> contour; ///< Level line
#endif

    // Tree structure
    LsShape* parent;  ///< Smallest containing shape
    LsShape* sibling; ///< Siblings are linked
    LsShape* child;   ///< First child

    // To move in the tree, taking into account that some shapes are ignored
    LsShape* find_parent();
    LsShape* find_child();
    LsShape* find_sibling();
    LsShape* find_prev_sibling();
};

/// To walk the tree in pre- or post-order
class LsTreeIterator {
public:
    typedef enum { Pre, Post } Order;
    LsTreeIterator();
    LsTreeIterator(Order ord, LsShape* shape);

    LsShape* operator*() const;
    bool operator==(const LsTreeIterator& it) const;
    bool operator!=(const LsTreeIterator& it) const;
    LsTreeIterator& operator++();
    static LsTreeIterator end(Order ord, LsShape* shape);
private:
    LsTreeIterator(Order ord, LsShape* shape, bool /*dummy*/);
    static LsShape* go_bottom(LsShape* shape);
    static LsShape* uncle(LsShape* shape);
    LsShape* s;
    Order o;
};

inline LsTreeIterator::LsTreeIterator()
: s(0), o(Pre) {}

inline LsTreeIterator::LsTreeIterator(Order ord, LsShape* shape)
: s(shape), o(ord) {
    if(ord == Post && s && ! s->bIgnore)
        s = go_bottom(s);
}

inline bool LsTreeIterator::operator==(const LsTreeIterator& it) const
{ return (s == it.s); }

inline bool LsTreeIterator::operator!=(const LsTreeIterator& it) const
{ return !(*this == it); }

inline LsShape* LsTreeIterator::operator*() const
{ return s; }

#endif
