/******************************************************************************
 * Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
 * Modifications Copyright© 2019 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
#pragma once

#if THRUST_DEVICE_COMPILER == THRUST_DEVICE_COMPILER_HCC
#include <thrust/system/hip/config.h>
// This include is needed to circumvent issue with thrust::pointer
// not having a value_type member when passing to a rocPRIM function
#include <thrust/system/hip/pointer.h>

#include <thrust/detail/type_traits/iterator/is_output_iterator.h>
#include <thrust/detail/type_traits/result_of_adaptable_function.h>
#include <thrust/distance.h>
#include <thrust/system/hip/detail/parallel_for.h>
#include <thrust/system/hip/detail/util.h>

// rocPRIM include
#include <rocprim/rocprim.hpp>

BEGIN_NS_THRUST
namespace hip_rocprim
{
namespace __transform
{
    struct no_stencil_tag
    {
    };

    template <class InputIt,
              class OutputIt,
              class StencilIt,
              class TransformOp,
              class Predicate>
    struct unary_transform_f
    {
        InputIt     input;
        OutputIt    output;
        StencilIt   stencil;
        TransformOp op;
        Predicate   pred;

        THRUST_HIP_FUNCTION
        unary_transform_f(InputIt     input_,
                          OutputIt    output_,
                          StencilIt   stencil_,
                          TransformOp op_,
                          Predicate   pred_)
            : input(input_)
            , output(output_)
            , stencil(stencil_)
            , op(op_)
            , pred(pred_)
        {
        }

        template <class Size>
        void THRUST_HIP_DEVICE_FUNCTION operator()(Size idx)
        {
            if(pred(raw_reference_cast(stencil[idx])))
                output[idx] = op(raw_reference_cast(input[idx]));
        }
    }; // struct unary_transform_stencil_f

    template <class InputIt, class OutputIt, class TransformOp, class Predicate>
    struct unary_transform_f<InputIt, OutputIt, no_stencil_tag, TransformOp, Predicate>
    {
        InputIt     input;
        OutputIt    output;
        TransformOp op;
        Predicate   pred;

        THRUST_HIP_FUNCTION
        unary_transform_f(
            InputIt input_, OutputIt output_, no_stencil_tag, TransformOp op_, Predicate pred_)
            : input(input_)
            , output(output_)
            , op(op_)
            , pred(pred_)
        {
        }

        template <class Size>
        void THRUST_HIP_DEVICE_FUNCTION operator()(Size idx)
        {
            if(pred(raw_reference_cast(input[idx])))
                output[idx] = op(raw_reference_cast(input[idx]));
        }
    }; // struct unary_transform_f

    template <class InputIt1,
              class InputIt2,
              class OutputIt,
              class StencilIt,
              class TransformOp,
              class Predicate>
    struct binary_transform_f
    {
        InputIt1    input1;
        InputIt2    input2;
        OutputIt    output;
        StencilIt   stencil;
        TransformOp op;
        Predicate   pred;

        THRUST_HIP_FUNCTION
        binary_transform_f(InputIt1    input1_,
                           InputIt2    input2_,
                           OutputIt    output_,
                           StencilIt   stencil_,
                           TransformOp op_,
                           Predicate   pred_)
            : input1(input1_)
            , input2(input2_)
            , output(output_)
            , stencil(stencil_)
            , op(op_)
            , pred(pred_)
        {
        }

        template <class Size>
        void THRUST_HIP_DEVICE_FUNCTION operator()(Size idx)
        {
            if(pred(raw_reference_cast(stencil[idx])))
                output[idx]
                    = op(raw_reference_cast(input1[idx]), raw_reference_cast(input2[idx]));
        }
    }; // struct binary_transform_stencil_f

    template <class InputIt1,
              class InputIt2,
              class OutputIt,
              class TransformOp,
              class Predicate>
    struct binary_transform_f<InputIt1,
                              InputIt2,
                              OutputIt,
                              no_stencil_tag,
                              TransformOp,
                              Predicate>
    {
        InputIt1    input1;
        InputIt2    input2;
        OutputIt    output;
        TransformOp op;
        Predicate   pred;

        THRUST_HIP_FUNCTION
        binary_transform_f(InputIt1 input1_,
                           InputIt2 input2_,
                           OutputIt output_,
                           no_stencil_tag,
                           TransformOp op_,
                           Predicate   pred_)
            : input1(input1_)
            , input2(input2_)
            , output(output_)
            , op(op_)
            , pred(pred_)
        {
        }

        template <class Size>
        void THRUST_HIP_DEVICE_FUNCTION operator()(Size idx)
        {
            if(pred(raw_reference_cast(input1[idx])))
                output[idx]
                    = op(raw_reference_cast(input1[idx]), raw_reference_cast(input2[idx]));
        }
    }; // struct binary_transform_f

    template <class Policy,
              class InputIt,
              class Size,
              class OutputIt,
              class StencilIt,
              class TransformOp,
              class Predicate>
    OutputIt THRUST_HIP_FUNCTION
    unary(Policy&     policy,
          InputIt     items,
          OutputIt    result,
          Size        num_items,
          StencilIt   stencil,
          TransformOp transform_op,
          Predicate   predicate)
    {
        if(num_items == 0)
            return result;

        typedef unary_transform_f<InputIt, OutputIt, StencilIt, TransformOp, Predicate>
            unary_transform_t;

        hip_rocprim::parallel_for(
            policy,
            unary_transform_t(items, result, stencil, transform_op, predicate),
            num_items);
        return result + num_items;
    }

    template <class Policy,
              class InputIt1,
              class InputIt2,
              class Size,
              class OutputIt,
              class StencilIt,
              class TransformOp,
              class Predicate>
    OutputIt THRUST_HIP_FUNCTION
    binary(Policy&     policy,
           InputIt1    items1,
           InputIt2    items2,
           OutputIt    result,
           Size        num_items,
           StencilIt   stencil,
           TransformOp transform_op,
           Predicate   predicate)
    {
        if(num_items == 0)
            return result;

        typedef binary_transform_f<InputIt1,
                                   InputIt2,
                                   OutputIt,
                                   StencilIt,
                                   TransformOp,
                                   Predicate>
            binary_transform_t;

        hip_rocprim::parallel_for(
            policy,
            binary_transform_t(items1, items2, result, stencil, transform_op, predicate),
            num_items);
        return result + num_items;
    }

} // namespace __transform

//-------------------------
// Thrust API entry points
//-------------------------

//-------------------------
//  one input data stream
//-------------------------

template <class Derived,
          class InputIt,
          class OutputIt,
          class StencilInputIt,
          class TransformOp,
          class Predicate>
OutputIt THRUST_HIP_FUNCTION
transform_if(execution_policy<Derived>& policy,
             InputIt                    first,
             InputIt                    last,
             StencilInputIt             stencil,
             OutputIt                   result,
             TransformOp                transform_op,
             Predicate                  predicate)
{
    typedef typename iterator_traits<InputIt>::difference_type size_type;
    size_type num_items = static_cast<size_type>(thrust::distance(first, last));
    return __transform::unary(
        policy, first, result, num_items, stencil, transform_op, predicate
    );
} // func transform_if

template <class Derived, class InputIt, class OutputIt, class TransformOp, class Predicate>
OutputIt THRUST_HIP_FUNCTION
transform_if(execution_policy<Derived>& policy,
             InputIt                    first,
             InputIt                    last,
             OutputIt                   result,
             TransformOp                transform_op,
             Predicate                  predicate)
{
    return hip_rocprim::transform_if(
        policy, first, last, __transform::no_stencil_tag(), result, transform_op, predicate
    );
} // func transform_if

template <class Derived, class InputIt, class OutputIt, class TransformOp>
OutputIt THRUST_HIP_FUNCTION
transform(execution_policy<Derived>& policy,
          InputIt                    first,
          InputIt                    last,
          OutputIt                   result,
          TransformOp                transform_op)
{
    typedef typename iterator_traits<InputIt>::difference_type size_type;
    size_type num_items = static_cast<size_type>(thrust::distance(first, last));

    if(num_items == 0)
        return result;

    // struct workaround is required for HIP-clang
    // THRUST_HIP_PRESERVE_KERNELS_WORKAROUND is required for HCC
    struct workaround
    {
        __host__
        static OutputIt par(execution_policy<Derived>& policy,
                            InputIt                    first,
                            InputIt                    last,
                            OutputIt                   result,
                            TransformOp                transform_op)
        {
            size_type num_items    = static_cast<size_type>(thrust::distance(first, last));
#if __HCC__ && __HIP_DEVICE_COMPILE__
            THRUST_HIP_PRESERVE_KERNELS_WORKAROUND(
                (rocprim::transform<rocprim::default_config, InputIt, OutputIt, TransformOp>)
            );
            (void)policy;
            (void)transform_op;
#else
            hipStream_t stream     = hip_rocprim::stream(policy);
            bool        debug_sync = THRUST_HIP_DEBUG_SYNC_FLAG;
            hipError_t  status
                = rocprim::transform(first, result, num_items, transform_op, stream, debug_sync);
            hip_rocprim::throw_on_error(status, "transform failed");
#endif
            return result + num_items;
        }

        __device__
        static OutputIt seq(execution_policy<Derived>& policy,
                            InputIt                    first,
                            InputIt                    last,
                            OutputIt                   result,
                            TransformOp                transform_op)
        {
            (void)policy;
            while(first != last)
            {
                *result++ = transform_op(raw_reference_cast(*first++));
            }
            return result;
        }
    };

#if __THRUST_HAS_HIPRT__
    return workaround::par(policy, first, last, result, transform_op);
#else
    return workaround::seq(policy, first, last, result, transform_op);
#endif
} // func transform

//-------------------------
// two input data streams
//-------------------------

template <class Derived,
          class InputIt1,
          class InputIt2,
          class StencilInputIt,
          class OutputIt,
          class TransformOp,
          class Predicate>
OutputIt THRUST_HIP_FUNCTION
transform_if(execution_policy<Derived>& policy,
             InputIt1                   first1,
             InputIt1                   last1,
             InputIt2                   first2,
             StencilInputIt             stencil,
             OutputIt                   result,
             TransformOp                transform_op,
             Predicate                  predicate)
{
    typedef typename iterator_traits<InputIt1>::difference_type size_type;
    size_type num_items = static_cast<size_type>(thrust::distance(first1, last1));
    return __transform::binary(
        policy, first1, first2, result, num_items, stencil, transform_op, predicate
    );
} // func transform_if

template <class Derived, class InputIt1, class InputIt2, class OutputIt, class TransformOp>
OutputIt THRUST_HIP_FUNCTION
transform(execution_policy<Derived>& policy,
          InputIt1                   first1,
          InputIt1                   last1,
          InputIt2                   first2,
          OutputIt                   result,
          TransformOp                transform_op)
{
    typedef typename iterator_traits<InputIt1>::difference_type size_type;
    size_type num_items = static_cast<size_type>(thrust::distance(first1, last1));

    if(num_items == 0)
        return result;

    THRUST_HIP_PRESERVE_KERNELS_WORKAROUND(
        (rocprim::transform<rocprim::default_config,
                            InputIt1,
                            InputIt2,
                            OutputIt,
                            TransformOp>)
    );
#if __THRUST_HAS_HIPRT__
    {
        hipStream_t stream     = hip_rocprim::stream(policy);
        bool        debug_sync = THRUST_HIP_DEBUG_SYNC_FLAG;
        hipError_t  status     = rocprim::transform(
            first1, first2, result, num_items, transform_op, stream, debug_sync);
        hip_rocprim::throw_on_error(status, "transform failed");

        return result + num_items;
    }
#else
    {
        (void)policy;
        while(first1 != last1)
        {
            *result++
                = transform_op(raw_reference_cast(*first1++), raw_reference_cast(*first2++));
        }
        return result;
    }
#endif
} // func transform

} // namespace hip_rocprim

END_NS_THRUST
#endif
