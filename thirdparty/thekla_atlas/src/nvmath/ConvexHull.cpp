// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "ConvexHull.h"

#include "Vector.inl"

#include "nvcore/RadixSort.h"
#include "nvcore/Array.inl"

#include <float.h>

using namespace nv;


// Compute the convex hull using Graham Scan.
void nv::convexHull(const Array<Vector2> & input, Array<Vector2> & output, float epsilon/*=0*/)
{
    const uint inputCount = input.count();

    Array<float> coords;
    coords.resize(inputCount);

    for (uint i = 0; i < inputCount; i++) {
        coords[i] = input[i].x;
    }

    RadixSort radix;
    radix.sort(coords);

    const uint * ranks = radix.ranks();

    Array<Vector2> top(inputCount);
    Array<Vector2> bottom(inputCount);

    Vector2 P = input[ranks[0]];
    Vector2 Q = input[ranks[inputCount-1]];

    float topy = max(P.y, Q.y);
    float boty = min(P.y, Q.y);

    for (uint i = 0; i < inputCount; i++) {
        Vector2 p = input[ranks[i]];
        if (p.y >= boty) top.append(p);
    }

    for (uint i = 0; i < inputCount; i++) {
        Vector2 p = input[ranks[inputCount-1-i]];
        if (p.y <= topy) bottom.append(p);
    }

    // Filter top list.
    output.clear();
    output.append(top[0]);
    output.append(top[1]);

    for (uint i = 2; i < top.count(); ) {
        Vector2 a = output[output.count()-2];
        Vector2 b = output[output.count()-1];
        Vector2 c = top[i];

        float area = triangleArea(a, b, c); // * 0.5

        if (area >= -epsilon) {
            output.popBack();
        }

        if (area < -epsilon || output.count() == 1) {
            output.append(c);
            i++;
        }
    }
    
    uint top_count = output.count();
    output.append(bottom[1]);

    // Filter bottom list.
    for (uint i = 2; i < bottom.count(); ) {
        Vector2 a = output[output.count()-2];
        Vector2 b = output[output.count()-1];
        Vector2 c = bottom[i];

        float area = triangleArea(a, b, c); // * 0.5

        if (area >= -epsilon) {
            output.popBack();
        }

        if (area < -epsilon || output.count() == top_count) {
            output.append(c);
            i++;
        }
    }

    // Remove duplicate element.
    nvDebugCheck(output.front() == output.back());
    output.popBack();
}


void nv::reduceConvexHullToNSides(Array<Vector2> *input, uint num_sides) {

    while (input->size() > num_sides) {
        // find the shortest side and merge it with its shorter neighbor.
        // we'll see how this goes... if it's not satisfactory, we could also merge
        // based partially on the "flatness" of adjacent edges.
        int shortest_index = 0;
        float shortest_length = nv::length((*input)[1] - (*input)[0]);

        for (uint i = 1; i < input->size(); i++) {
            nv::Vector2 p1 = (*input)[i];
            nv::Vector2 p2 = (*input)[(i + 1) % input->size()];
            float length = nv::length(p2 - p1);

            if (length < shortest_length) {
                shortest_index = i;
                shortest_length = length;
            }
        }

        int prev_index = (shortest_index + input->size() - 1) % input->size();
        int next_index = (shortest_index + 1) % input->size();
        int after_index = (next_index + 1) % input->size();
        float prev_len = nv::length((*input)[prev_index] - (*input)[shortest_index]);
        float next_len = nv::length((*input)[next_index] - (*input)[after_index]);

        if (prev_len < next_len) {
            input->removeAt(shortest_index);
        } else {
            input->removeAt(next_index);
        }
    }
}

bool nv::isClockwise(const Array<Vector2> &input) {
    nv::Vector2 d1 = input[1] - input[0];
    nv::Vector2 d2 = input[2] - input[0];
    nv::Vector3 normal = cross(Vector3(d1.x, d1.y, 0), Vector3(d2.x, d2.y, 0));
    return (normal.z < 0.0f);
}

void nv::flipWinding(Array<Vector2> *input) {
    int start = 0;
    int end = input->size() - 1;

    while (end > start) {
        nv::Vector2 swap = (*input)[start];
        (*input)[start] = (*input)[end];
        (*input)[end] = swap;
        start++;
        end--;
    }
}



/*
void testConvexHull() {

    Array<Vector2> points;
    points.append(Vector2(1.00, 1.00));
    points.append(Vector2(0.00, 0.00));
    points.append(Vector2(1.00, 1.00));
    points.append(Vector2(1.00, -1.00));
    points.append(Vector2(2.00, 5.00));
    points.append(Vector2(-5.00, 3.00));
    points.append(Vector2(-4.00, -3.00));
    points.append(Vector2(7.00, -4.00));

    Array<Vector2> hull;
    convexHull(points, hull);

}
*/



struct Line {
    Vector2 v;
    Vector2 d;
};


bool intersect(const Line &line0, const Line &line1, Vector2 * point)
{
	float d = triangleArea(line0.d, line1.d);
	if (fabsf(d) < NV_EPSILON) // Parallel lines
		return false;

	float t = triangleArea(line1.d, line0.v - line1.v) / d;

	if (t < 0.5f) // Intersects on the wrong side
		return false;

	*point = line0.v + t * line0.d;
	return true;
}

static inline Line buildLine(const Array<Vector2> & vertices, uint i0, uint i1) {
    Line line;
    line.v = vertices[i0];
    line.d = vertices[i1] - vertices[i0]; // @@ Normalize?
    return line;
}


static bool buildPolygon(const Array<Vector2> & vertices, const Array<uint> & edges, Array<Vector2> & output) {

    const uint vertexCount = vertices.count();
    const uint edgeCount = edges.count();

    output.clear();
    output.reserve(edgeCount);

    // Intersect each edge against the next.
    Line first, prev, current;
    Vector2 point;
    
    first = prev = buildLine(vertices, 0, 1);

    for (uint i = 1; i < edgeCount; i++) {
        current = buildLine(vertices, edges[i], (edges[i]+1) % vertexCount);

        if (!intersect(prev, current, &point)) {
            return false;
        }
        output.append(point);

        prev = current;
    }

    if (!intersect(current, first, &point)) {
        return false;
    }
    output.append(point);

    return true;
}

// Triangulate polygon and sum triangle areas.
static float polygonArea(const Array<Vector2> & vertices) {

    float area = 0;
    Vector2 v0 = vertices[0];

    for (uint i = 2; i < vertices.count(); i++)
    {
        Vector2 v1 = vertices[i];
        Vector2 v2 = vertices[i-1]; 

        area += triangleArea(v0, v1, v2);
    }

    return area * 0.5f;
}

// @@ Test this!
static bool next(Array<uint> & edges, uint vertexCount) {
    int count = edges.count();

    uint previous = vertexCount;

    for (int i = count-1; i >= 0; i--) {
        uint e = edges[i];
        if (e + 1 < previous) {
            // increment current and reset remainder of the sequence.
            for (; i < count; i++) {
                edges[i] = ++e;
            }
            return true;
        }
        previous = e;
    }

    return false;
}



// Given a convex hull, finds best fit polygon with the given number of edges.
// Brute force implementation, tries all possible edges, but may not arrive to best result, because edges of best fit polygon are not necessarily aligned to convex hull edges. 
bool nv::bestFitPolygon(const Array<Vector2> & hullVertices, uint edgeCount, Array<Vector2> * bestPolygon) {
    uint vertexCount = hullVertices.count();

    // Not a valid polygon.
    if (edgeCount <= 2) return false;

    // Hull has same or less vertices that desired polygon.
    if (edgeCount >= vertexCount) return false;

    bestPolygon->reserve(edgeCount);
    float bestArea = FLT_MAX;

    if (edgeCount == 4) {
        // Try with axis aligned box first.
        Vector2 box_min(FLT_MAX);
        Vector2 box_max(-FLT_MAX);

        for (uint i = 0; i < hullVertices.count(); i++) {
            box_min.x = min(box_min.x, hullVertices[i].x);
            box_min.y = min(box_min.y, hullVertices[i].y);
            box_max.x = max(box_max.x, hullVertices[i].x);
            box_max.y = max(box_max.y, hullVertices[i].y);
        }

        bestArea = (box_max.x - box_min.x) * (box_max.y - box_min.y);

        bestPolygon->append(Vector2(box_min.x, box_min.y));
        bestPolygon->append(Vector2(box_min.x, box_max.y));
        bestPolygon->append(Vector2(box_max.x, box_max.y));
        bestPolygon->append(Vector2(box_max.x, box_min.y));
    }


    // Select all possible combinations of consecutive edges.

    // For example, for groups of 3 out of 5 we want:

    // 012
    // 013
    // 014
    // 023
    // 024
    // 034
    // 123
    // 124
    // 134


    Array<uint> edges(edgeCount);

    // Init edge sequence.
    for (uint i = 0; i < edgeCount; i++) {
        edges.append(i);
    }

    // Traverse sequence testing all 
    Array<Vector2> polygon(edgeCount);
    do {
        if (buildPolygon(hullVertices, edges, polygon)) {
            float area = polygonArea(polygon);
            if (area < bestArea) {
                bestArea = area;
                *bestPolygon = polygon;
            }
        }
    } while (next(edges, vertexCount));

    return bestArea < FLT_MAX;
}


static bool pointInTriangle(const Vector2 & p, const Vector2 & a, const Vector2 & b, const Vector2 & c)
{
    return triangleArea(a, b, p) >= 0.00001f && 
        triangleArea(b, c, p) >= 0.00001f && 
        triangleArea(c, a, p) >= 0.00001f; 
}

void nv::triangulate(const Array<Vector2> & input, Array<uint> * output) {

    const uint edgeCount = input.count();
    nvDebugCheck(edgeCount >= 3);

    output->clear();    // @@ Do we want to clear?
    output->reserve(edgeCount);

    if (edgeCount == 3) {
        // Simple case for triangles.
        output->append(0);
        output->append(1);
        output->append(2);
    }
    else {
        Array<int> polygonVertices;
        Array<float> polygonAngles;

        polygonVertices.resize(edgeCount);
        polygonAngles.resize(edgeCount);

        for (uint i = 0; i < edgeCount; ++i) {
            polygonVertices[i] = i;
        }

        while (polygonVertices.size() > 2) {
            uint size = polygonVertices.size();

            // Update polygon angles. @@ Update only those that have changed.
            float minAngle = 2 * PI;
            uint bestEar = 0; // Use first one if none of them is valid.
            bool bestIsValid = false;
            for (uint i = 0; i < size; i++) {
                uint i0 = i; 
                uint i1 = (i+1) % size; // @@ Use Sean's polygon interation trick.
                uint i2 = (i+2) % size;

                Vector2 p0 = input[polygonVertices[i0]];
                Vector2 p1 = input[polygonVertices[i1]];
                Vector2 p2 = input[polygonVertices[i2]];

                float d = clamp(dot(p0-p1, p2-p1) / (length(p0-p1) * length(p2-p1)), -1.0f, 1.0f);
                float angle = acosf(d);
                
                float area = triangleArea(p0, p1, p2);
                if (area < 0.0f) angle = 2.0f * PI - angle;

                polygonAngles[i1] = angle;

                if (angle < minAngle || !bestIsValid) {

                    // Make sure this is a valid ear, if not, skip this point.
                    bool valid = true;
                    for (uint j = 0; j < size; j++) {
                        if (j == i0 || j == i1 || j == i2) continue;
                        Vector2 p = input[polygonVertices[j]];

                        if (pointInTriangle(p, p0, p1, p2)) {
                            valid = false;
                            break;
                        }
                    }

                    if (valid || !bestIsValid) {
                        minAngle = angle;
                        bestEar = i1;
                        bestIsValid = valid;
                    }
                }
            }

            nvDebugCheck(minAngle <= 2 * PI);

            // Clip best ear:

            uint i0 = (bestEar+size-1) % size;
            uint i1 = (bestEar+0) % size;
            uint i2 = (bestEar+1) % size;

            int v0 = polygonVertices[i0];
            int v1 = polygonVertices[i1];
            int v2 = polygonVertices[i2];
            
            output->append(v0);
            output->append(v1);
            output->append(v2);

            polygonVertices.removeAt(i1);
            polygonAngles.removeAt(i1);
        }
    }

}
