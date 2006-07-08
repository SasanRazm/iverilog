/*
 * Copyright (c) 1999 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#ifdef HAVE_CVS_IDENT
#ident "$Id: PDelays.cc,v 1.15 2006/07/08 21:48:46 steve Exp $"
#endif

# include "config.h"

# include  <iostream>

# include  "PDelays.h"
# include  "PExpr.h"
# include  "verinum.h"

PDelays::PDelays()
{
      delete_flag_ = true;
      for (unsigned idx = 0 ;  idx < 3 ;  idx += 1)
	    delay_[idx] = 0;
}

PDelays::~PDelays()
{
      if (delete_flag_) {
	    for (unsigned idx = 0 ;  idx < 3 ;  idx += 1)
		  delete delay_[idx];
      }
}

void PDelays::set_delay(PExpr*del)
{
      assert(del);
      assert(delay_[0] == 0);
      delay_[0] = del;
      delete_flag_ = true;
}


void PDelays::set_delays(const svector<PExpr*>*del, bool df)
{
      assert(del);
      assert(del->count() <= 3);
      for (unsigned idx = 0 ;  idx < del->count() ;  idx += 1)
	    delay_[idx] = (*del)[idx];

      delete_flag_ = df;
}

static NetExpr*calculate_val(Design*des, NetScope*scope, const PExpr*expr)
{

      NetExpr*dex = expr->elaborate_expr(des, scope, -1, false);
      if (NetExpr*tmp = dex->eval_tree()) {
	    delete dex;
	    dex = tmp;
      }

	/* If the delay expression is a real constant or vector
	   constant, then evaluate it, scale it to the local time
	   units, and return an adjusted value. */

      if (NetECReal*tmp = dynamic_cast<NetECReal*>(dex)) {
	    verireal fn = tmp->value();

	    int shift = scope->time_unit() - des->get_precision();
	    long delay = fn.as_long(shift);
	    if (delay < 0)
		  delay = 0;

	    delete tmp;
	    NetEConst*tmp2 = new NetEConst(verinum(delay));
	    tmp2->set_line(*expr);
	    return tmp2;
      }


      if (NetEConst*tmp = dynamic_cast<NetEConst*>(dex)) {
	    verinum fn = tmp->value();

	    unsigned long delay =
		  des->scale_to_precision(fn.as_ulong(), scope);

	    delete tmp;
	    NetEConst*tmp2 = new NetEConst(verinum(delay));
	    tmp2->set_line(*expr);
	    return tmp2;
      }

	/* Oops, cannot evaluate down to a constant. */
      return dex;
}

static NetExpr* make_delay_nets(Design*des, NetExpr*expr)
{
      if (dynamic_cast<NetESignal*> (expr))
	    return expr;

      if (dynamic_cast<NetEConst*> (expr))
	    return expr;

      NetNet*sig = expr->synthesize(des);
      if (sig == 0) {
	    cerr << expr->get_line() << ": error: Expression " << *expr
		 << " is not suitable for delay expression." << endl;
	    return 0;
      }

      expr = new NetESignal(sig);
      return expr;
}

void PDelays::eval_delays(Design*des, NetScope*scope,
			  NetExpr*&rise_time,
			  NetExpr*&fall_time,
			  NetExpr*&decay_time,
			  bool as_nets_flag) const
{
      assert(scope);


      if (delay_[0]) {
	    rise_time = calculate_val(des, scope, delay_[0]);
	    if (as_nets_flag)
		  rise_time = make_delay_nets(des, rise_time);

	    if (delay_[1]) {
		  fall_time = calculate_val(des, scope, delay_[1]);
		  if (as_nets_flag)
			fall_time = make_delay_nets(des, fall_time);

		  if (delay_[2]) {
			decay_time = calculate_val(des, scope, delay_[2]);
		  if (as_nets_flag)
			decay_time = make_delay_nets(des, decay_time);

		  } else {
			if (rise_time < fall_time)
			      decay_time = rise_time;
			else
			      decay_time = fall_time;
		  }
	    } else {
		  assert(delay_[2] == 0);
		  fall_time = rise_time;
		  decay_time = rise_time;
	    }
      } else {
	    rise_time = 0;
	    fall_time = 0;
	    decay_time = 0;
      }
}

/*
 * $Log: PDelays.cc,v $
 * Revision 1.15  2006/07/08 21:48:46  steve
 *  Handle real valued literals in net contexts.
 *
 * Revision 1.14  2006/06/02 04:48:49  steve
 *  Make elaborate_expr methods aware of the width that the context
 *  requires of it. In the process, fix sizing of the width of unary
 *  minus is context determined sizes.
 *
 * Revision 1.13  2006/01/03 05:22:14  steve
 *  Handle complex net node delays.
 *
 * Revision 1.12  2006/01/02 05:33:19  steve
 *  Node delays can be more general expressions in structural contexts.
 *
 * Revision 1.11  2003/06/21 01:21:42  steve
 *  Harmless fixup of warnings.
 *
 * Revision 1.10  2003/02/08 19:49:21  steve
 *  Calculate delay statement delays using elaborated
 *  expressions instead of pre-elaborated expression
 *  trees.
 *
 *  Remove the eval_pexpr methods from PExpr.
 *
 * Revision 1.9  2002/08/12 01:34:58  steve
 *  conditional ident string using autoconfig.
 *
 * Revision 1.8  2001/12/29 20:19:31  steve
 *  Do not delete delay expressions of UDP instances.
 *
 * Revision 1.7  2001/11/22 06:20:59  steve
 *  Use NetScope instead of string for scope path.
 *
 * Revision 1.6  2001/11/07 04:01:59  steve
 *  eval_const uses scope instead of a string path.
 *
 * Revision 1.5  2001/07/25 03:10:48  steve
 *  Create a config.h.in file to hold all the config
 *  junk, and support gcc 3.0. (Stephan Boettcher)
 *
 * Revision 1.4  2001/01/20 02:15:50  steve
 *  apologize for not supporting non-constant delays.
 *
 * Revision 1.3  2001/01/14 23:04:55  steve
 *  Generalize the evaluation of floating point delays, and
 *  get it working with delay assignment statements.
 *
 *  Allow parameters to be referenced by hierarchical name.
 *
 * Revision 1.2  2000/02/23 02:56:53  steve
 *  Macintosh compilers do not support ident.
 *
 * Revision 1.1  1999/09/04 19:11:46  steve
 *  Add support for delayed non-blocking assignments.
 *
 */

