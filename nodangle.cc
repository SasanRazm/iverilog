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
#if !defined(WINNT) && !defined(macintosh)
#ident "$Id: nodangle.cc,v 1.14 2002/02/02 06:13:38 steve Exp $"
#endif

# include "config.h"

/*
 * This functor scans the design looking for dangling objects and
 * excess local signals. These deletions are not necessarily required
 * for proper functioning of anything, but they can clean up the
 * appearance of design files that are generated.
 */
# include  "functor.h"
# include  "netlist.h"

class nodangle_f  : public functor_t {
    public:
      void event(Design*des, NetEvent*ev);
      void signal(Design*des, NetNet*sig);

      unsigned count_;
};

void nodangle_f::event(Design*des, NetEvent*ev)
{
      if (NetEvent*match = ev->find_similar_event()) {
	    assert(match != ev);
	    ev->replace_event(match);
      }

      if (ev->nwait() != 0)
	    return;

      if (ev->ntrig() != 0)
	    return;

      delete ev;
}

void nodangle_f::signal(Design*des, NetNet*sig)
{
	/* Cannot delete signals referenced in an expression. */
      if (sig->get_eref() > 0)
	    return;

	/* Cannot delete the ports of tasks or functions. There are
	   too many places where they are referenced. */
      if ((sig->port_type() != NetNet::NOT_A_PORT)
	  && (sig->scope()->type() == NetScope::TASK))
	    return;

      if ((sig->port_type() != NetNet::NOT_A_PORT)
	  && (sig->scope()->type() == NetScope::FUNC))
	    return;

	/* Check to see if the signal is completely unconnected. If
	   all the bits are unlinked, then delete it. */
      unsigned unlinked = 0;
      for (unsigned idx =  0 ;  idx < sig->pin_count() ;  idx += 1)
	    if (! sig->pin(idx).is_linked()) unlinked += 1;

      if (unlinked == sig->pin_count()) {
	    delete sig;
	    return;
      }

	/* The remaining things can only be done to synthesized
	   signals, not ones that appear in the original Verilog. */
      if (! sig->local_flag())
	    return;

	/* Check to see if there is some significant signal connected
	   to every pin of this signal. */
      unsigned significant_flags = 0;
      for (unsigned idx = 0 ;  idx < sig->pin_count() ;  idx += 1) {
	    Nexus*nex = sig->pin(idx).nexus();

	    for (Link*cur = nex->first_nlink()
		       ; cur ;  cur = cur->next_nlink()) {

		  if (cur == &sig->pin(idx))
			continue;

		  NetNet*cursig = dynamic_cast<NetNet*>(cur->get_obj());
		  if (cursig == 0)
			continue;

		  if (cursig->local_flag())
			continue;

		  significant_flags += 1;
		  break;
	    }
      }

	/* If every pin is connected to another significant signal,
	   then I can delete this one. */
      if (significant_flags == sig->pin_count()) {
	    count_ += 1;
	    delete sig;
      }
}

void nodangle(Design*des)
{
      nodangle_f fun;

      do {
	    fun.count_ = 0;
	    des->functor(&fun);
      } while (fun.count_ > 0);
}

/*
 * $Log: nodangle.cc,v $
 * Revision 1.14  2002/02/02 06:13:38  steve
 *  event find_similar should not find self.
 *
 * Revision 1.13  2001/07/27 02:41:55  steve
 *  Fix binding of dangling function ports. do not elide them.
 *
 * Revision 1.12  2001/07/25 03:10:49  steve
 *  Create a config.h.in file to hold all the config
 *  junk, and support gcc 3.0. (Stephan Boettcher)
 *
 * Revision 1.11  2001/02/17 05:14:35  steve
 *  Cannot elide task ports.
 *
 * Revision 1.10  2000/11/19 20:48:53  steve
 *  Killing some signals might make others killable.
 *
 * Revision 1.9  2000/11/18 05:12:45  steve
 *  Delete unreferenced signals no matter what.
 *
 * Revision 1.8  2000/06/25 19:59:42  steve
 *  Redesign Links to include the Nexus class that
 *  carries properties of the connected set of links.
 *
 * Revision 1.7  2000/05/31 02:26:49  steve
 *  Globally merge redundant event objects.
 *
 * Revision 1.6  2000/05/07 04:37:56  steve
 *  Carry strength values from Verilog source to the
 *  pform and netlist for gates.
 *
 *  Change vvm constants to use the driver_t to drive
 *  a constant value. This works better if there are
 *  multiple drivers on a signal.
 *
 * Revision 1.5  2000/04/28 21:00:29  steve
 *  Over agressive signal elimination in constant probadation.
 *
 * Revision 1.4  2000/04/18 04:50:20  steve
 *  Clean up unneeded NetEvent objects.
 *
 * Revision 1.3  2000/02/23 02:56:55  steve
 *  Macintosh compilers do not support ident.
 *
 * Revision 1.2  1999/11/28 23:42:02  steve
 *  NetESignal object no longer need to be NetNode
 *  objects. Let them keep a pointer to NetNet objects.
 *
 * Revision 1.1  1999/11/18 03:52:20  steve
 *  Turn NetTmp objects into normal local NetNet objects,
 *  and add the nodangle functor to clean up the local
 *  symbols generated by elaboration and other steps.
 *
 */

