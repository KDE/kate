// This file is part of the Kate project within KDE.
// (C) 2013 Alex Turbov <i.zaufi at gmail.com>
// License: LGPL v2 or v3

/**
 * Class to simplify (a little, cuz JS have no RAII) work with
 * \c document.editBegin() and \c document.editEnd().
 *
 * Sometimes your function may have a really complicated logic
 * when doing some modifications on a current document.
 * For example:
 * \code
 *  if (smth)
 *  {
 *      // do edit
 *  }
 *  if (smth_else)
 *  {
 *      // do more or first edit
 *  }
 *  else if (some)
 *      return;
 *  // do another modifications
 * \endcode
 * And that would be good to have all possible modifications withing one undo action...
 * To do so, just add \c EditSession instance at top level scope and call
 * \c editBegin() when you want to modify smth. Next calls will be "ignored" if
 * edit session has started. Then you've done, call \c editEnd() to "commit" possible
 * modifications if any has took place, otherwise nothing happened.
 *
 * So modified example will look like this:
 * \code
 *  var tr = EditSession();
 *  if (smth)
 *  {
 *      tr.editBegin();
 *      // do edit
 *  }
 *  if (smth_else)
 *  {
 *      // do more or first edit
 *  }
 *  else if (some)
 *  {
 *      tr.editEnd();
 *      return;
 *  }
 * // do another modifications
 * tr.editEnd();
 * \endcode
 */
function EditSession()
{
    this.started = false;
}

EditSession.prototype.editBegin = function()
{
    if (!this.started)
    {
        document.editBegin();
        this.started = true;
    }
}

EditSession.prototype.editEnd = function()
{
    if (this.started)
    {
        document.editEnd();
        this.started = false;
    }
}

// kate: indent-width 4; replace-tabs on;
