Installing EDL
##############

Prerequisites:
==============

To install EDL you will need Lex & Yacc (or the newer version Flex and Bison).

Install Process:
================

Follow this steps to install EDL:

#. Get the sources of EDL, either form the source repository or a source archive
#. Enter the EDL source directory:

   .. code-block:: shell

      cd edl

#. Run the configure script:

   .. code-block:: shell

      ./configure

#. Compile using ``make``:

   .. code-block:: shell

      make

   once the compilation os over, a binary named ``edl-ester`` should be in the
   ``src`` subdirectory.

#. Optional run ``make install`` to install edl's parsers to:

   .. code-block:: shell

      make install

