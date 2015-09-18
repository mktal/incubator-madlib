/* ----------------------------------------------------------------------- *//**
 *
 * @file linear.cpp
 *
 * @brief average variance functions
 *
 *//* ----------------------------------------------------------------------- */

#include <dbconnector/dbconnector.hpp>
#include <modules/shared/HandleTraits.hpp>
#include "avg_var.hpp"

namespace madlib {

namespace modules {

namespace hello_world {

template <class Handle>
class AvgVarTransitionState {
	template <class OtherHandle>
    friend class AvgVarTransitionState;

  public:
    AvgVarTransitionState(const AnyType &inArray)
        : mStorage(inArray.getAs<Handle>()) {

        rebind();
    }

    /**
     * @brief Convert to backend representation
     *
     * We define this function so that we can use State in the
     * argument list and as a return type.
     */
    inline operator AnyType() const {
        return mStorage;
    }

    /**
     * @brief Merge with another State object by copying the intra-iteration
     *     fields
     */
    template <class OtherHandle>
    AvgVarTransitionState &operator+=(
        const AvgVarTransitionState<OtherHandle> &inOtherState) {

        if (mStorage.size() != inOtherState.mStorage.size())
            throw std::logic_error("Internal error: Incompatible transition "
                                   "states");

        numRows += inOtherState.numRows;
        sum += inOtherState.sum;
        return *this;
    }

  private:
    void rebind() {
        sum.rebind(&mStorage[0]);
        numRows.rebind(&mStorage[1]);
    }

    Handle mStorage;

  public:
    typename HandleTraits<Handle>::ReferenceToDouble sum;
    typename HandleTraits<Handle>::ReferenceToUInt64 numRows;
};


AnyType
avg_var_transition::run(AnyType& args) {
	// state[0]: sum
	// state[1]: nElems
    AvgVarTransitionState<MutableArrayHandle<double> > state = args[0];
    double x = args[1].getAs<double>();
    state.sum += x;
    state.numRows ++;
    return state;
}

AnyType
avg_var_merge_states::run(AnyType& args) {
    AvgVarTransitionState<MutableArrayHandle<double> > stateLeft = args[0];
    AvgVarTransitionState<ArrayHandle<double> > stateRight = args[1];

    // Merge states together and return
    stateLeft += stateRight;
    return stateLeft;
}

AnyType
avg_var_final::run(AnyType& args) {
    AvgVarTransitionState<MutableArrayHandle<double> > state = args[0];

    // If we haven't seen any data, just return Null. This is the standard
    // behavior of aggregate function on empty data sets (compare, e.g.,
    // how PostgreSQL handles sum or avg on empty inputs)
    if (state.numRows == 0)
        return Null();

    state.sum = state.sum / state.numRows;
    return state;
}
// -----------------------------------------------------------------------

} // namespace regress

} // namespace modules

} // namespace madlib
