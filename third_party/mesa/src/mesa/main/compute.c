/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "util/glheader.h"
#include "bufferobj.h"
#include "context.h"
#include "state.h"
#include "api_exec_decl.h"

#include "pipe/p_state.h"

#include "state_tracker/st_context.h"
#include "state_tracker/st_cb_bitmap.h"
#include "state_tracker/st_util.h"

static bool
check_valid_to_compute(struct gl_context *ctx, const char *function)
{
   if (!_mesa_has_compute_shaders(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "unsupported function (%s) called",
                  function);
      return false;
   }

   /* From the OpenGL 4.3 Core Specification, Chapter 19, Compute Shaders:
    *
    * "An INVALID_OPERATION error is generated if there is no active program
    *  for the compute shader stage."
    */
   if (ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE] == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(no active compute shader)",
                  function);
      return false;
   }

   return true;
}

static bool
validate_DispatchCompute(struct gl_context *ctx, struct pipe_grid_info *info)
{
   if (!check_valid_to_compute(ctx, "glDispatchCompute"))
      return GL_FALSE;

   for (int i = 0; i < 3; i++) {
      /* From the OpenGL 4.3 Core Specification, Chapter 19, Compute Shaders:
       *
       * "An INVALID_VALUE error is generated if any of num_groups_x,
       *  num_groups_y and num_groups_z are greater than or equal to the
       *  maximum work group count for the corresponding dimension."
       *
       * However, the "or equal to" portions appears to be a specification
       * bug. In all other areas, the specification appears to indicate that
       * the number of workgroups can match the MAX_COMPUTE_WORK_GROUP_COUNT
       * value. For example, under DispatchComputeIndirect:
       *
       * "If any of num_groups_x, num_groups_y or num_groups_z is greater than
       *  the value of MAX_COMPUTE_WORK_GROUP_COUNT for the corresponding
       *  dimension then the results are undefined."
       *
       * Additionally, the OpenGLES 3.1 specification does not contain "or
       * equal to" as an error condition.
       */
      if (info->grid[i] > ctx->Const.MaxComputeWorkGroupCount[i]) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDispatchCompute(num_groups_%c)", 'x' + i);
         return GL_FALSE;
      }
   }

   /* The ARB_compute_variable_group_size spec says:
    *
    * "An INVALID_OPERATION error is generated by DispatchCompute if the active
    *  program for the compute shader stage has a variable work group size."
    */
   struct gl_program *prog = ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE];
   if (prog->info.workgroup_size_variable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glDispatchCompute(variable work group size forbidden)");
      return GL_FALSE;
   }

   return GL_TRUE;
}

static bool
validate_DispatchComputeGroupSizeARB(struct gl_context *ctx,
                                     struct pipe_grid_info *info)
{
   if (!check_valid_to_compute(ctx, "glDispatchComputeGroupSizeARB"))
      return GL_FALSE;

   /* The ARB_compute_variable_group_size spec says:
    *
    * "An INVALID_OPERATION error is generated by
    *  DispatchComputeGroupSizeARB if the active program for the compute
    *  shader stage has a fixed work group size."
    */
   struct gl_program *prog = ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE];
   if (!prog->info.workgroup_size_variable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glDispatchComputeGroupSizeARB(fixed work group size "
                  "forbidden)");
      return GL_FALSE;
   }

   for (int i = 0; i < 3; i++) {
      /* The ARB_compute_variable_group_size spec says:
       *
       * "An INVALID_VALUE error is generated if any of num_groups_x,
       *  num_groups_y and num_groups_z are greater than or equal to the
       *  maximum work group count for the corresponding dimension."
       */
      if (info->grid[i] > ctx->Const.MaxComputeWorkGroupCount[i]) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDispatchComputeGroupSizeARB(num_groups_%c)", 'x' + i);
         return GL_FALSE;
      }

      /* The ARB_compute_variable_group_size spec says:
       *
       * "An INVALID_VALUE error is generated by DispatchComputeGroupSizeARB if
       *  any of <group_size_x>, <group_size_y>, or <group_size_z> is less than
       *  or equal to zero or greater than the maximum local work group size
       *  for compute shaders with variable group size
       *  (MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB) in the corresponding
       *  dimension."
       *
       * However, the "less than" is a spec bug because they are declared as
       * unsigned integers.
       */
      if (info->block[i] == 0 ||
          info->block[i] > ctx->Const.MaxComputeVariableGroupSize[i]) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDispatchComputeGroupSizeARB(group_size_%c)", 'x' + i);
         return GL_FALSE;
      }
   }

   /* The ARB_compute_variable_group_size spec says:
    *
    * "An INVALID_VALUE error is generated by DispatchComputeGroupSizeARB if
    *  the product of <group_size_x>, <group_size_y>, and <group_size_z> exceeds
    *  the implementation-dependent maximum local work group invocation count
    *  for compute shaders with variable group size
    *  (MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB)."
    */
   uint64_t total_invocations = info->block[0] * info->block[1];
   if (total_invocations <= UINT32_MAX) {
      /* Only bother multiplying the third value if total still fits in
       * 32-bit, since MaxComputeVariableGroupInvocations is also 32-bit.
       */
      total_invocations *= info->block[2];
   }
   if (total_invocations > ctx->Const.MaxComputeVariableGroupInvocations) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDispatchComputeGroupSizeARB(product of local_sizes "
                  "exceeds MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB "
                  "(%u * %u * %u > %u))",
                  info->block[0], info->block[1], info->block[2],
                  ctx->Const.MaxComputeVariableGroupInvocations);
      return GL_FALSE;
   }

   /* The NV_compute_shader_derivatives spec says:
    *
    * "An INVALID_VALUE error is generated by DispatchComputeGroupSizeARB if
    *  the active program for the compute shader stage has a compute shader
    *  using the "derivative_group_quadsNV" layout qualifier and
    *  <group_size_x> or <group_size_y> is not a multiple of two.
    *
    *  An INVALID_VALUE error is generated by DispatchComputeGroupSizeARB if
    *  the active program for the compute shader stage has a compute shader
    *  using the "derivative_group_linearNV" layout qualifier and the product
    *  of <group_size_x>, <group_size_y>, and <group_size_z> is not a multiple
    *  of four."
    */
   if (prog->info.cs.derivative_group == DERIVATIVE_GROUP_QUADS &&
       ((info->block[0] & 1) || (info->block[1] & 1))) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDispatchComputeGroupSizeARB(derivative_group_quadsNV "
                  "requires group_size_x (%d) and group_size_y (%d) to be "
                  "divisble by 2)", info->block[0], info->block[1]);
      return GL_FALSE;
   }

   if (prog->info.cs.derivative_group == DERIVATIVE_GROUP_LINEAR &&
       total_invocations & 3) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDispatchComputeGroupSizeARB(derivative_group_linearNV "
                  "requires product of group sizes (%"PRIu64") to be divisible "
                  "by 4)", total_invocations);
      return GL_FALSE;
   }

   return GL_TRUE;
}

static bool
valid_dispatch_indirect(struct gl_context *ctx,  GLintptr indirect)
{
   GLsizei size = 3 * sizeof(GLuint);
   const uint64_t end = (uint64_t) indirect + size;
   const char *name = "glDispatchComputeIndirect";

   if (!check_valid_to_compute(ctx, name))
      return GL_FALSE;

   /* From the OpenGL 4.3 Core Specification, Chapter 19, Compute Shaders:
    *
    * "An INVALID_VALUE error is generated if indirect is negative or is not a
    *  multiple of four."
    */
   if (indirect & (sizeof(GLuint) - 1)) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s(indirect is not aligned)", name);
      return GL_FALSE;
   }

   if (indirect < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s(indirect is less than zero)", name);
      return GL_FALSE;
   }

   /* From the OpenGL 4.3 Core Specification, Chapter 19, Compute Shaders:
    *
    * "An INVALID_OPERATION error is generated if no buffer is bound to the
    *  DRAW_INDIRECT_BUFFER binding, or if the command would source data
    *  beyond the end of the buffer object."
    */
   if (!ctx->DispatchIndirectBuffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s: no buffer bound to DISPATCH_INDIRECT_BUFFER", name);
      return GL_FALSE;
   }

   if (_mesa_check_disallowed_mapping(ctx->DispatchIndirectBuffer)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(DISPATCH_INDIRECT_BUFFER is mapped)", name);
      return GL_FALSE;
   }

   if (ctx->DispatchIndirectBuffer->Size < end) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(DISPATCH_INDIRECT_BUFFER too small)", name);
      return GL_FALSE;
   }

   /* The ARB_compute_variable_group_size spec says:
    *
    * "An INVALID_OPERATION error is generated if the active program for the
    *  compute shader stage has a variable work group size."
    */
   struct gl_program *prog = ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE];
   if (prog->info.workgroup_size_variable) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "%s(variable work group size forbidden)", name);
      return GL_FALSE;
   }

   return GL_TRUE;
}

static void
prepare_compute(struct gl_context *ctx)
{
   struct st_context *st = st_context(ctx);

   st_flush_bitmap_cache(st);
   st_invalidate_readpix_cache(st);

   if (ctx->NewState)
      _mesa_update_state(ctx);

   st_validate_state(st, ST_PIPELINE_COMPUTE_STATE_MASK);
}

static ALWAYS_INLINE void
dispatch_compute(GLuint num_groups_x, GLuint num_groups_y,
                 GLuint num_groups_z, bool no_error)
{
   GET_CURRENT_CONTEXT(ctx);
   struct pipe_grid_info info = { 0 };

   FLUSH_VERTICES(ctx, 0, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDispatchCompute(%d, %d, %d)\n",
                  num_groups_x, num_groups_y, num_groups_z);

   info.grid[0] = num_groups_x;
   info.grid[1] = num_groups_y;
   info.grid[2] = num_groups_z;

   if (!no_error && !validate_DispatchCompute(ctx, &info))
      return;

   if (num_groups_x == 0u || num_groups_y == 0u || num_groups_z == 0u)
       return;

   struct gl_program *prog =
      ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE];
   info.block[0] = prog->info.workgroup_size[0];
   info.block[1] = prog->info.workgroup_size[1];
   info.block[2] = prog->info.workgroup_size[2];

   prepare_compute(ctx);
   ctx->pipe->launch_grid(ctx->pipe, &info);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH)
      _mesa_flush(ctx);
}

void GLAPIENTRY
_mesa_DispatchCompute_no_error(GLuint num_groups_x, GLuint num_groups_y,
                               GLuint num_groups_z)
{
   dispatch_compute(num_groups_x, num_groups_y, num_groups_z, true);
}

void GLAPIENTRY
_mesa_DispatchCompute(GLuint num_groups_x,
                      GLuint num_groups_y,
                      GLuint num_groups_z)
{
   dispatch_compute(num_groups_x, num_groups_y, num_groups_z, false);
}

static ALWAYS_INLINE void
dispatch_compute_indirect(GLintptr indirect, bool no_error)
{
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDispatchComputeIndirect(%ld)\n", (long) indirect);

   if (!no_error && !valid_dispatch_indirect(ctx, indirect))
      return;

   struct pipe_grid_info info = { 0 };
   info.indirect_offset = indirect;
   info.indirect = ctx->DispatchIndirectBuffer->buffer;

   struct gl_program *prog =
      ctx->_Shader->CurrentProgram[MESA_SHADER_COMPUTE];
   info.block[0] = prog->info.workgroup_size[0];
   info.block[1] = prog->info.workgroup_size[1];
   info.block[2] = prog->info.workgroup_size[2];

   prepare_compute(ctx);
   ctx->pipe->launch_grid(ctx->pipe, &info);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH)
      _mesa_flush(ctx);
}

extern void GLAPIENTRY
_mesa_DispatchComputeIndirect_no_error(GLintptr indirect)
{
   dispatch_compute_indirect(indirect, true);
}

extern void GLAPIENTRY
_mesa_DispatchComputeIndirect(GLintptr indirect)
{
   dispatch_compute_indirect(indirect, false);
}

static ALWAYS_INLINE void
dispatch_compute_group_size(GLuint num_groups_x, GLuint num_groups_y,
                            GLuint num_groups_z, GLuint group_size_x,
                            GLuint group_size_y, GLuint group_size_z,
                            bool no_error)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx,
                  "glDispatchComputeGroupSizeARB(%d, %d, %d, %d, %d, %d)\n",
                  num_groups_x, num_groups_y, num_groups_z,
                  group_size_x, group_size_y, group_size_z);

   struct pipe_grid_info info = { 0 };
   info.grid[0] = num_groups_x;
   info.grid[1] = num_groups_y;
   info.grid[2] = num_groups_z;

   info.block[0] = group_size_x;
   info.block[1] = group_size_y;
   info.block[2] = group_size_z;

   if (!no_error &&
       !validate_DispatchComputeGroupSizeARB(ctx, &info))
      return;

   if (num_groups_x == 0u || num_groups_y == 0u || num_groups_z == 0u)
       return;

   prepare_compute(ctx);
   ctx->pipe->launch_grid(ctx->pipe, &info);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH)
      _mesa_flush(ctx);
}

void GLAPIENTRY
_mesa_DispatchComputeGroupSizeARB_no_error(GLuint num_groups_x,
                                           GLuint num_groups_y,
                                           GLuint num_groups_z,
                                           GLuint group_size_x,
                                           GLuint group_size_y,
                                           GLuint group_size_z)
{
   dispatch_compute_group_size(num_groups_x, num_groups_y, num_groups_z,
                               group_size_x, group_size_y, group_size_z,
                               true);
}

void GLAPIENTRY
_mesa_DispatchComputeGroupSizeARB(GLuint num_groups_x, GLuint num_groups_y,
                                  GLuint num_groups_z, GLuint group_size_x,
                                  GLuint group_size_y, GLuint group_size_z)
{
   dispatch_compute_group_size(num_groups_x, num_groups_y, num_groups_z,
                               group_size_x, group_size_y, group_size_z,
                               false);
}