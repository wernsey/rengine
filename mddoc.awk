#! /usr/bin/awk -f 
# Generates Markdown from comments in source code for the wiki on GitHub
#
# To use the script, invoke it like so:
#   > awk -f mddoc.awk src/luastate.c > output.md
# and then paste the contents of output.md in GitHub's wiki editor.

# Excuse this script if it looks strange:
# This script is modified from an older script that I used to generate HTML 
# from code to generate GitHub-flavoured Markdown.
# Some of the code in here may look useless, but I've tried to keep it 
# backwards compatible with my previous script.

BEGIN { 
}

/\/\*/ { comment = 1; }
/--\[\[/ { comment = 2; }

/\*1/ { if(!comment) next; s = substr($0, index($0, "*1") + 2); print "# " filter(s); next;}
/\*2/ { if(!comment) next; s = substr($0, index($0, "*2") + 2); print "## " filter(s); next;}
/\*3/ { if(!comment) next; s = substr($0, index($0, "*3") + 2); print "### " filter(s); next;}
/\*@/ { if(!comment) next; s = substr($0, index($0, "*@") + 2); print "#### " filter(s); next;}
/\*#[ \t\r]*$/ { if(!comment) next; if(!pre) print "\n\n"; next;}
/\*#/ { if(!comment) next; s = substr($0, index($0, "*#") + 2); print filter(s);}
/\*&/ { if(!comment) next; s = substr($0, index($0, "*&") + 2); print "`" filter(s) "`"; next;}
/\*X/ { if(!comment) next; s = substr($0, index($0, "*X") + 2); print "\n**Example:**\n```\n" filter(s) "\n```"; next;}
/\*N/ { if(!comment) next; s = substr($0, index($0, "*N") + 2); print "\n**Note:**" filter(s) ""; next;}
/\*\[/ { if(!comment) next; pre=1; print "```"; next;}
/\*]/ { if(!comment) next; pre=0; print "```"; next;}
/\*\{/ { if(!comment) next; print ""; next;}
/\*\*/ { if(!comment) next; s = substr($0, index($0, "**") + 2); print "* " filter(s); next;}

/^[ \t]*\*}/ { if(!comment) next; print "\n"; next;}

/\*-/ { if(!comment) next; print "***"; next;}
/\*=/ { if(!comment) next; print "***"; next;}

/\*\// { if(comment == 1) comment=0; }
/\]\]/ { if(comment == 2) comment=0; }

END { }

function filter(ss,        j, k1, k2, k3)
{
	gsub(/\\n[ \t\r]*$/, "\n", ss);
	gsub(/{{/, "`", ss); 
	gsub(/}}/, "`", ss);
	gsub(/{\*/, "**", ss); 
	gsub(/\*}/, "**", ss);
	gsub(/{\//, "_", ss); 
	gsub(/\/}/, "_", ss);
	
	# Markdown doesn't have underline, so I use italics instead.
	gsub(/{_/, "_", ss); 
	gsub(/_}/, "_", ss);	
		
	# My HTML script had a syntax for links within the document
	# which I don't support in the markdown version.
	while(j = match(ss, /##[A-Za-z0-9_]+/)) {
		k1 = substr(ss, 1, j - 1);
		k2 = substr(ss, j + 2, RLENGTH-2);
		k3 = substr(ss, j + RLENGTH);
		ss = k1 k2 k3
	}	
	# Ditto
	while(j = match(ss, /~~[A-Za-z0-9_]+/)) {
		k1 = substr(ss, 1, j - 1);
		k2 = substr(ss, j + 2, RLENGTH-2);
		k3 = substr(ss, j + RLENGTH);
		ss = k1 k2 k3
	}
	
	gsub(/\*\//, "", ss);
	return ss;
}
