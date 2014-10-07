/***
@module posix
*/
/*
 * POSIX library for Lua 5.1/5.2.
 * (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2014
 * (c) Reuben Thomas <rrt@sc3d.org> 2010-2013
 * (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
 * Clean up and bug fixes by Leo Razoumov <slonik.az@gmail.com> 2006-10-11
 * Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br> 07 Apr 2006 23:17:49
 * Based on original by Claudio Terra for Lua 3.x.
 * With contributions by Roberto Ierusalimschy.
 * With documentation from Steve Donovan 2012
 */

#if _POSIX_VERSION >= 200112L

#include <config.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#include "_helpers.h"


/***
System Message Functions.
@section sysmsg
*/


/***
Get a message queue identifier
@function msgget
@int key message queue id, or `IPC_PRIVATE` for a new queue
@int[opt=0] flags bitwise or of `IPC_CREAT` and `IPC_EXCL`
@string[opt="rw-rw-rw-"] mode execute bits are ignored, otherwise see @{chmod} for format.
@treturn[1] int message queue identifier, if successful
@return[2] nil
@treturn[2] string error message
@see msgget(2)
*/
static int
Pmsgget(lua_State *L)
{
	mode_t mode;
	key_t key = checkint(L, 1);
	int msgflg = optint(L, 2, 0);
	const char *modestr = optstring(L, 3,"rw-rw-rw-");

	checknargs (L, 3);
	if (mode_munch(&mode, modestr))
		luaL_argerror(L, 2, "bad mode");
	msgflg |= mode;

	return pushresult(L, msgget(key, msgflg), NULL);
}


/***
Send message to a message queue
@function msgsnd
@int id message queue identifier returned by @{msgget}
@int type arbitrary message type
@string message content
@int[opt=0] flags optionally `IPC_NOWAIT`
@treturn int 0, if successful
@return[2] nil
@treturn[2] string error message
@see msgsnd(2)
 */
static int
Pmsgsnd(lua_State *L)
{
	void *ud;
	lua_Alloc lalloc = lua_getallocf(L, &ud);
	struct {
		long mtype;
		char mtext[0];
	} *msg;
	size_t len;
	size_t msgsz;
	ssize_t r;

	int msgid = checkint(L, 1);
	long msgtype = checklong(L, 2);
	const char *msgp = luaL_checklstring(L, 3, &len);
	int msgflg = optint(L, 4, 0);

	checknargs(L, 4);

	msgsz = sizeof(long) + len;

	if ((msg = lalloc(ud, NULL, 0, msgsz)) == NULL)
		return pusherror(L, "lalloc");

	msg->mtype = msgtype;
	memcpy(msg->mtext, msgp, len);

	r = msgsnd(msgid, msg, msgsz, msgflg);
	lua_pushinteger(L, r);

	lalloc(ud, msg, msgsz, 0);

	return (r == -1 ? pusherror(L, NULL) : 1);
}


/***
Receive message from a message queue
@function msgrcv
@int id message queue identifier returned by @{msgget}
@int size maximum message size
@int type message type (optional, default - 0)
@int[opt=0] flags bitwise OR of zero or more of `IPC_NOWAIT`, `MSG_EXCEPT`
  and `MSG_NOERROR`
@treturn[1] int message type from @{msgsnd}
@treturn[1] string message text, if successful
@return[2] nil
@treturn[2] string error message
@see msgrcv(2)
 */
static int
Pmsgrcv(lua_State *L)
{
	int msgid = checkint(L, 1);
	size_t msgsz = checkint(L, 2);
	long msgtyp = optint(L, 3, 0);
	int msgflg = optint(L, 4, 0);

	void *ud;
	lua_Alloc lalloc;
	struct {
		long mtype;
		char mtext[0];
	} *msg;

	checknargs(L, 4);
	lalloc = lua_getallocf(L, &ud);

	if ((msg = lalloc(ud, NULL, 0, msgsz)) == NULL)
		return pusherror(L, "lalloc");

	int res = msgrcv(msgid, msg, msgsz, msgtyp, msgflg);
	if (res != -1)
	{
		lua_pushinteger(L, msg->mtype);
		lua_pushlstring(L, msg->mtext, res - sizeof(long));
	}
	lalloc(ud, msg, msgsz, 0);

	return (res == -1) ? pusherror(L, NULL) : 2;
}


void
sysmsg_setconst (lua_State *L)
{
	PCONST( IPC_CREAT	);
	PCONST( IPC_EXCL	);
	PCONST( IPC_PRIVATE	);
	PCONST( IPC_NOWAIT	);

#ifdef MSG_EXCEPT
	PCONST( MSG_EXCEPT	);
#endif
#ifdef MSG_NOERROR
	PCONST( MSG_NOERROR	);
#endif
}

#endif