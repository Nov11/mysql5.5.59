### why MySQL 5.5?
This is the earliest version that I can find on official web site with a cmake support.

I'm going to use this version as code base to apply examples from 'uderstanding mysql internals'.

The version mentioned in that book is 5.1. There is no cmake support back then. And I'm not proficient in setting up
IDE on a makefile build system. While 5.7 has a source code tar ball twice the size of 5.5. I'll just pick 5.5 for now.


### adopting example from chapter 7
* code is in storage/oreilly-csv directory
* init/done method must be provide for 'show engines;' will not list the new engine.
* no need to create a handleton as the book states in 5.1. filling some fields of the parameter in init function is enough.
* use postfix with 'open' and base extension accordingly
* 'store_lock' is needed but external_lock is not
* build command line: 'cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=XXX -DMYSQL_DATADIR=XXX/data && make -j 4 && make install
'
* 'use test' ,create a table with 'engine = your_engine_name' ,place a file named after the format 'table.your_postfix' in test folder of 'data'(i.e. XXX/data) directory with some content and finally 'select * from table_name' should return the content in the file
* './bin/mysqladmin -u root shutdown' in base directory will shutdown mysqld faster than terminate directly in IDE
### helpful links
[mysql internal manual](https://dev.mysql.com/doc/internals/en/creating-handlerton.html)
[mysql installation from source code](http://howtolamp.com/lamp/mysql/5.6/installing#post-install)
=================================================================

MySQL Server 5.5

This is a release of MySQL, a dual-license SQL database server.
For the avoidance of doubt, this particular copy of the software 
is released under the version 2 of the GNU General Public License. 
MySQL is brought to you by Oracle.

Copyright (c) 2000, 2018, Oracle and/or its affiliates. All rights reserved.

License information can be found in the COPYING file.

MySQL FOSS License Exception
We want free and open source software applications under certain
licenses to be able to use specified GPL-licensed MySQL client
libraries despite the fact that not all such FOSS licenses are
compatible with version 2 of the GNU General Public License.  
Therefore there are special exceptions to the terms and conditions 
of the GPLv2 as applied to these client libraries, which are 
identified and described in more detail in the FOSS License 
Exception at 
<http://www.mysql.com/about/legal/licensing/foss-exception.html>.

This distribution may include materials developed by third
parties. For license and attribution notices for these
materials, please refer to the documentation that accompanies
this distribution (see the "Licenses for Third-Party Components"
appendix) or view the online documentation at 
<http://dev.mysql.com/doc/>.

GPLv2 Disclaimer
For the avoidance of doubt, except that if any license choice
other than GPL or LGPL is available it will apply instead, 
Oracle elects to use only the General Public License version 2 
(GPLv2) at this time for any software where a choice of GPL 
license versions is made available with the language indicating 
that GPLv2 or any later version may be used, or where a choice 
of which version of the GPL is applied is otherwise unspecified.

For further information about MySQL or additional documentation, 
see:
- The latest information about MySQL: http://www.mysql.com
- The current MySQL documentation: http://dev.mysql.com/doc

Some Reference Manual sections of special interest:
- If you are migrating from an older version of MySQL, please 
  read the "Upgrading from..." section.
- To see what MySQL can do, take a look at the features section.
- For installation instructions, see the Installing and Upgrading
  chapter.
- For the new features/bugfix history, see the MySQL Change History 
  appendix.

You can browse the MySQL Reference Manual online or download it 
in any of several formats at the URL given earlier in this file.
Source distributions include a local copy of the manual in the
Docs directory.
