#=======================================================
#
# the SIMPL - Self Extracting Archive 
#
#=======================================================

SIMPLVER=3.3.4

MYPWD=`pwd`
if [ $MYPWD != '/tmp' ]
then
	echo "==================================================="
	echo " This script needs to be run from /tmp."
	echo ""
	echo " Please copy it there and rerun from /tmp."
	echo ""
	echo "==================================================="
	exit
fi

echo "==================================================="
echo ""
echo "           SIMPL Self Extracting Archive"
echo ""
echo " This archive will be safely installed entirely in"
echo " /tmp. With the option to permanently install SIMPL"
echo " into a directory of your choosing."
echo ""
echo " You can examine this installer script with any text"
echo " editor. Nothing is hidden.  The gzip'd tarballs at"
echo " the end of this file are all individually available"
echo " from the SIMPL project website at"
echo " http://www.icanprogram.com/simpl"
echo ""
echo " As with all open source software we offer this script"
echo " without warranty or implied liabilities."
echo ""
echo "==================================================="
echo ""
echo -n "I accept these terms [y/n] "
read ans
if [ $ans == 'n' ]
then
	exit
fi

#
# SKIP denotes the line number where the tarball begins.
#
SKIP=`awk '/^__TARBALL_FOLLOWS__/ { print NR + 1; exit 0; }' $0`
THIS=`pwd`/$0


echo ""
echo "==================================================="
echo ""
echo " STAGE 1: Setting up work area in /tmp."
echo ""
echo " This SIMPL install will be compiled and run from "
echo " /tmp."
echo " Several files and subdirectories will be created"
echo " including:"
echo " /tmp/simpl.config - working config file"
echo " /tmp/simplfifo - working SIMPL sandbox"
echo " /tmp/simpl - SIMPL tree"
echo " /tmp/simpl/simplBook - sample code tree"
echo ""
echo "==================================================="
echo ""
echo "******* Press Enter to continue ********"
read ans
echo ""

#
#  Create the contents of the temporary config file.
#  This will be appended to more lines to form the cut and paste
#  insert for the users startup profile should they elect to 
#  make a permanent installation.
#
TMP_CONFIG=/tmp/simpl.config

echo "if [ -z \$FIFO_PATH ]" > $TMP_CONFIG
echo "then" >> $TMP_CONFIG
echo "	if [ ! -d /tmp/simplfifo ]" >> $TMP_CONFIG
echo "	then" >> $TMP_CONFIG
echo "		mkdir /tmp/simplfifo" >> $TMP_CONFIG
echo "		chmod a+rw /tmp/simplfifo" >> $TMP_CONFIG
echo "	fi" >> $TMP_CONFIG
echo "	export FIFO_PATH=/tmp/simplfifo" >> $TMP_CONFIG
echo "fi" >> $TMP_CONFIG
echo "export PATH=\$PATH:\$SIMPL_HOME/bin:\$SIMPL_HOME/scripts:." >> $TMP_CONFIG
echo "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:\$SIMPL_HOME/lib" >> $TMP_CONFIG

#
#  Create the working directories in /tmp.
#
if [ ! -d /tmp/simplfifo ]
then
	mkdir /tmp/simplfifo
	chmod a+rw /tmp/simplfifo
fi
export FIFO_PATH=/tmp/simplfifo

if [ -h /tmp/simpl ]
then
	cd /tmp
	rm simpl 
fi
ln -s simpl-$SIMPLVER simpl 

export SIMPL_HOME=/tmp/simpl

export PATH=$PATH:$SIMPL_HOME/bin:$SIMPL_HOME/scripts:.

export TEST_HOME=$SIMPL_HOME

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/tmp/simpl/lib

#
#  Display the relevant temporary SIMPL environment variables
#
echo " temporary SIMPL environment variables"
echo ""
echo "FIFO_PATH=$FIFO_PATH"
echo "SIMPL_HOME=$SIMPL_HOME"
echo "TEST_HOME=$TEST_HOME"
echo "PATH=$PATH"
echo ""
echo "Code will be temporarily installed at $SIMPL_HOME"
echo ""
echo "******* Press Enter to continue ********"
read ans
echo ""

echo ""
echo "==================================================="
echo ""
echo " STAGE 2: Undoing the installation tarballs."
echo ""
echo " Several SIMPL tarballs are extracted into /tmp. "
echo " These include:"
echo " simpl-$SIMPLVER.tar.gz - main SIMPL source tarball."
echo " simpltest.tar.gz - SIMPL testing framework."
echo ""
echo "==================================================="
echo ""
echo "******* Press Enter to continue ********"
read ans
echo ""

#
#  Actual undoing of the self extracting archive occurs here
#
cd /tmp
pwd
tail -n +$SKIP $THIS | tar -xv

tar -zxvf /tmp/simplplugbin-$SIMPLVER.tar.gz
tar -zxvf /tmp/simpltest.tar.gz

cd /tmp/simpl/lib
ln -s libsimpl.so libsimpl.so.1
ln -s libsimpllog.so libsimpllog.so.1
ln -s libsimplmisc.so libsimplmisc.so.1

echo ""
echo "******* Press Enter to continue ********"
read ans
echo ""

cd /tmp

echo ""
echo "==================================================="
echo ""
echo " STAGE 3: Running the tests."
echo ""
echo " The testing framework associated with the sample "
echo " code for the book will be exercised next."
echo ""
echo "==================================================="
echo ""
echo "******* Press Enter to continue ********"
read ans
echo ""

count=3
while [ $count -gt 0 ]
do
	echo ""
	echo "==================================================="
	echo " List of Tests "
	echo " (You will be allowed $count more test runs.)" 
	echo ""

	seetest i
	echo ""
	echo -n "Which test do you wish to run? (suggest s0001) [q to exit] "
	read ans
	if [ $ans == 'q' ] 
	then
		break
	else
		echo ""
		pretest $ans
		dotest $ans $1
	fi
	let count=count-1
done

echo ""
echo "==================================================="
echo ""
echo " STAGE 4: Allowing this SIMPL installation"
echo "          to become permanent."
echo ""
echo " You will be asked to select a permanent directory "
echo " home for this SIMPL instance.    Once done the"
echo " contents of the /tmp/simpl tree will be moved to "
echo " this permanent home."
echo ""
echo " To make the new environment variables permanent" 
echo " you will have to cut and paste the contents of a"
echo " premade config file into your startup profile."
echo ""
echo "==================================================="
echo ""
echo "******* Press Enter to continue ********"
read ans
echo ""

echo -n "Would you like to install this instance of SIMPL permanently? [y/n] "
read ans
if [ $ans == 'y' ]
then
	count=3
	while [ $count -gt 0 ]
	do
		echo -n "Where would you like SIMPL installed? [eg. /home] "
		read ans

#
# Intercept a null entry and allow retry
#
		if [ ${#ans} == 0 ]
		then
			echo "Invalid entry. Please reenter a valid directory." 
		else
#
# Intercept a basename of simpl which will result in simpl/simpl
#
			MYSIMPL_DIR=$ans
			MYBASE=`basename $ans`
			if [ $MYBASE == "simpl" ]
			then
				echo "Please reenter a directory which doesn't end in simpl."
			else 
			
#
# Intercept existing simpl directory to prevent accidental overwrite
#
			if [ -d $MYSIMPL_DIR/simpl ]
			then
				echo "Cannot install here."
				echo "$MYSIMPL_DIR/simpl already exists."
				echo "Please reenter another directory."
			else
				break
			fi
			fi
		fi

		let count=count-1
		echo "You have $count tries left."
	done

#
#  check if all retries were used up.  If so exit.
#
	if [ $count -eq 0 ]
	then
		exit
	fi

	echo "MYSIMPL_DIR=$MYSIMPL_DIR"

#
#  The directory entered must itself exist.  Allow the user 
#  an opportunity to create it.
#
	let count=3
	while [ ! -d $MYSIMPL_DIR ]
	do
		echo "Please make sure"
		echo " $MYSIMPL_DIR"
		echo "exists."

		echo "Hit Enter to continue once you've completed this."
		read ans
		let count=count-1
		echo "You have $count tries left."
		if [ $count -eq 0 ] 
		then
			break
		fi
	done

#
#  If all conditions are met move the simpl tree to new location.   
#
	if [ -d $MYSIMPL_DIR ]
	then
		mv /tmp/simpl-$SIMPLVER $MYSIMPL_DIR
		cd $MYSIMPL_DIR
		ln -s simpl-$SIMPLVER simpl
		export SIMPL_HOME=$MYSIMPL_DIR/simpl
		SIMPL_CONFIG=$SIMPL_HOME/simpl.config

#
#  Create the startup profile insert.   User must cut and paste
#  this insert manually into the .profile or .bash_profile file.
#
		echo "#=====================================================" > $SIMPL_CONFIG
		echo "#" >> $SIMPL_CONFIG
		echo "# Append this to the end of your startup profile" >> $SIMPL_CONFIG
		echo "# in order that SIMPL environment variables are available" >> $SIMPL_CONFIG
		echo "# at each console." >> $SIMPL_CONFIG
		echo "#" >> $SIMPL_CONFIG
		echo "#=====================================================" >> $SIMPL_CONFIG
		echo "" >> $SIMPL_CONFIG

		echo "export SIMPL_HOME=$SIMPL_HOME" >> $SIMPL_CONFIG
		cat $TMP_CONFIG >> $SIMPL_CONFIG

#
#  Announce this to the user.
#

		echo ""
		echo "=============================================================="
		echo "Please manually append the contents of"
		echo "   $SIMPL_CONFIG"
		echo "to your startup profile (.profile or .bash_profile or .bashrc)."
		echo "=============================================================="
	fi
fi

exit 0
__TARBALL_FOLLOWS__
simplplugbin-3.3.4.tar.gz                                                                           0000644 0001750 0000144 00000250445 11345467366 014067  0                                                                                                    ustar   bob                             users                                                                                                                                                                                                                  � �n�K �[moG����_�X^P�HY�Y�=@��D���{��!h�4�Y���İ��WOUw�EY> ���-��L��{=U���j]���/=�=+���7����#��|�O^����|�g��������������Kz?9~u|�>�-�x�����Z3s�'��������@�J����·�W�g��c���Ef��^���r��7[����9_���e���&_ٱo��-Lc��|����*k����Tn��c�|S�U��\��ԛ�"q�Y�f�#������!)��M�_o�֏�[oK�X��S��Ȼ?���4����y����]�۪���5��0oWk~�����J�{7τ��#�tw������b�]��z��������u�q��W�M�qf�r�V6Y�D ��xJ�~qtofdY˕�o<oS����÷�Vyz�X�Ղ��G���Y]������d~m��O��i�O��[�fi�k[������mI����)w�1O�~�ӯ�u�U�N�;�N����S�� ��̆AOs�5�1v��=�o�ĩ����!����������˘��>���@�?�M�/_M)�O_$�}���A�S���~���G�Y��T�q�7�s�k<�2��WS��#}��t��t:��W�zz49R�����������@�o�G������/��L������}�?����������Q�����x��R_T$Ĳ4ȱ��n�{���t�S=��ѭ�V}�����?M�������p�JO&�'G�]�ڴ�ϙ����0���nT��9=�NO��NO^�M���m��w:�c��G�B�c^br��_�������$.1/�)�Ԩ�P�G�����P��e�wm�Y���j��9�ml��ޖs�^5� �0���+�RW��kW����fk�hjk�����&$Q�/�q��X�hRQSd:�J�nYdK,�����qu�UQ5N�&�B�k�&{���a�hI�S����ĵidI����b��Ň��M��!�B����Rg$��f�T��e�gV���^AX}�^5��AbX(*������&c���֓S���U�39O��1�x�+�^2L���Ʈt1畖��2?�Di��J�O�*����srtr�<l4=��i�K�A��vN��S��]��.���1߰��v>�٧2+��)��Z`&aH<$[^;�-Etę�.�}��������g����nik6��/�ռ�;&��*��;}�b��=<�[�O�(�15M`���Ǐ�Ϸ�Ŀ���cw�c��P���DB����8��������K�zEA]S�**��g�v�"��̊hM�z��v&Hz�<%k��ZX���o��X��^�Br����-�"�,�ڊ�a=bnE둴s[H�eQawH/:��v��r����b�,�����5�۠�����ڟv�e���A��x��7�@�yݒ��J{kK=�ݪ?�y�����db���������IcA������C��{K̐��\0���Pg�i4\�������YA9��U��==kaF�ŀ
8~Aڧ�۬�ӠU��4��j/�%Yl�j�j����Qm/���(�H�L��p-��,�ͭ)J�$uj�rt<��{sc��XId��H6D��Owsk�Hٲ���\۬[�J:��q<X��̯����1-&��d�ꅩ�_�W8Ap���P�yKp�%;�$��E�)!�$���Q�b�m�<Z:��wv�uf䅛5�@�tf-+�lF<Q�v�n%���"��SUT�vmj�*Ć��?&A Z��n��Nk�`ގ�,��%9��'���M�c��zx(�p�p�e��P����/�;��=�ۣ7��(nen�y��-B�T�&Atw89՟[�"��x[�ٟ�G(��*X�F��KW�$�� j��Ăg�����}��$�<�?Bs�;�`
�ɉz��T���� �_,�	���&C�Lׇ'�
��h:8a'k�'�J�b�?�I�.�l� E������'��F�ݹ�F��-���wo��7w ]K�3K�Q1��� м� �9����CZ��=��D��kjO:F��6E�8k��pP���tzgE�sG�
Iaf�)rY��$��w����ӯ��t~�=�\�������	��/�'��������Ç�������g������g���O�_^��
��������zh��x2���nQN^�~M��%��X,���9����t�bdyf��*�^'���Enԟ�G�����㣑���F�?��h:�L&�G�����L�s� �R�q�M#���RgN��.f�4vF[��r�J���ۄG�粥��tR5l�a�bW�߹�/|��j� Ki��z	����J�>��*�ϖ"�B����PgS�XI�����������#J8z���V�
���4���*��2�
g�
��F1H6���䔄��t0w��Ql�9�8:�����v!?i��7��jJ����� ��˸=�2��lAvF߆��6cB�4�A�o�P�D�
��0���*T���X��Y�h��ɝJ��"M ��k���&H�;�i��n�~"� h��$x(&�s�Wj�6KG��n��DVHM1�:aa,,�˾�|0N�R�!�\�y���a��b'5�'i�``���D�ن��I��]l��`#�aY[R��D9crU@�TLV�,y��/@�bm�\f��^AfĊ���k�fC�i���f[M���>����?0��A�F%�ǁ�3�^ܑp���"��ƦY�-mW��<�N�8��#�As��\�,l���D��Eӛ�1�'� *�"2c2y����L0cD����д��)j�wɝ�v��Oq)�o4藰s���`c�%��Ϋ��"WI�0xn�U��ޤ�d=6M�L�M,�q��/�8%�����vf|q��Q6`Ě^3��M!ё��S�\���h/CW./��&$�^�{��7���F�!��W��0=��
o���"6�^
Z�Q����X�$�/��x!Ӈ6�Y�3��h���Zc
�8N���%P���z)1��E�C�K�
�͸��ݓ�y�xI�,�ƔP��v�}��7�fN\!��U��mځ��Jq=��Hܱ��ɡ�Co�o*y[DۖHew���������W�T�����s�$K`
� �$�Oy��Sh���U������_څ)C#/��F}S�u9�y�������=Gd���7i'4R�e��j���'��Mh�J�L����;C_3d�LZ�&Ά, �-�/*/%e��M>�:@5J�=��� �$��(�50��VI�}�QN�����nfD�%k��X��2+�'խmj���b�[y��ͨ�c��* $�$D� A�n��~����v�
-)L!"�e��z ���{06B02�ՙ��$<˨No����	��+)K�����FTP�eB#��G'�1-	v[;
{�Q�W�) �la �(�͊;C��:��v�Bn�qMȄ�Fr�Gd�R�:z u#��B���m��@�1K�`f�*�E�ˑy�%ur�@pb;$�!���i�ԗ�B.�8H���W��Ҙ�Ux�!;�̋w�n��1�A9�dMdq�J)�n���Wh�6=á��[ H�\pRYc_Wߐ I�u��r�|1��uݓ�#ړ>�)�3�(+W! 骨���1��tZ&�J2�@��TQ[��M� �4��BPk�b���vN������3	��{��3�)G7�1�-&d�k��LS��i�ꁵ��:�	���g(n�x����\~K���2i[".Z>zh�;R"L��2uD\�;�N� ���$.�)�uB[����/T�QЉ�n{�_��X|ꬨ�v���Y�r�5�v^���p�#5�2
 ��1#cBqpa��:�`��06l�LZ :����Ej8��=\�3� �Q�[9D=~���ɾ��󦙿�x�,�备#��sư�B���=�F~�W$�깐���fM�J�FN<��u=�[���N�IU2L�>�g;_�;�N���D
�l��Z�";��*1��p�|U���$��ںui�5Kԭz��`����[������ޤ&��CeGz���!l��(x1�6,k��1bMZ
SFb&�����eQ��:�S2��zVR�j˧� �s1�K�Qӳ�Fp 67��/�8����'��U?�1%A\��8�AR*�p��L���� ߸���� �v���qt4�jnX�*/�S+�d��-��ٳx0
%��dP0�m�ЂQ4��d�3����QrR5h
�cr�Ys����q��j�M�:��#+�耔����<�5�5������C%�Cb��~U������+}��~���ۋ닏������맿^|�a��^\]���'���?��xw�����X�e��3rg9X�AG�T�=�fG�BA�$gP�?ŗ<"�����q��oQ�<�mi6���LW��r(Ќ�"C�/;ྴ����kz��=�s�����E�<Ǒ'�%�Q��'���e��af32[�EȘ���c�����" 7z���¥�[[��&o�v�R�S�"n.NPa�W�Y;�v�9��@w����G�I�t$��N�����gRBE:ީ�8�>�Ru9��-��1�\��#k^�wP�E���4�C���Bdl�˘_�++��
�+(��!x�c�.L%}�N`�}k�A�&Y�@�Q�4բ%HDb����!=4�Fi	�@�<��K���:��J&������"��U���%0�/�m$N��-�*Fs�~eh}y%.����L�/���=f�I��M8~OPh���;F��e�Y��{GX��N=�1�QY����q<�.�2�jWj��1
�P�o&�����ҷu� �,i$�ntE�}\��NC���8}�r�V���E�a��A����6zQ��bs����XHR��vPG�b�ȕ���k�����T�47��q7`�}��G:�A�Ղj������(\�Һ�v��G<�m��P�/� �����qVL�O���W�nq�̫��.@��6�E8�Zټh	���+�N} �5�j��RȠ�^�u]��l�}�_��jUoT/o����JS�Ɩ�w���5�U�D�*���i̎a��}B𦺥��h��x>�V*]�0ُ��gI%|�%��ɻ^n<z4�$4ƣ9�k��/t��-w����+��#Ζ�i�á"E�2A��LB_�W����3R�*d�f�2�]	��e�A�R��Ɏ;�!��@<�*07QfF��e{r����VnM
���y��F(a(�}�(�)R�WR�oက]gϻ!��p��G2iE�fRk��l����\�I�^���{K��%�C�e�Q��l'R��� ���rQ%wڃQ�+�E-`�C�}�$��q�EVm2nb�Nb��CY	�	ዑ|U�� ����!9g����.&�x�2�1�g5�y�z��X�߰���E�"孻A�i0b38�3z�a	_��9���R�:�e�1�-�C��EJc��ɤ��h@��@�<t��X�	�1V r+��3���c�ڮ�Rb!˛�e����'�Sg�^W�����o8G��J�d']Zb����H��.2���䪥�,�Lq�KK�ƎU�ٞ|�̦oeb_��"VI���e��!���4�}IoEh��8x	�d�rP�.��^��v�Jn�^<$�a��!��a�* Ez��)OřO���������g�EK��$ �zKc��Z����+������G
�SX"��R󝱎O��$�����.��5�O�U����o�����3�C	Ͽ0x�B��ch�5B�5f���;}v����"
�R���a�q�+�>r�6��Z�9P�pa��ah�-��P|��&�z���e�H֤�}n�ջ7¸�3�K����D�K�t�>c<�&--c�>l�U���������>�U^���{o�KZCE�Z�
��n� ��5)L�4	�Nf��$�1���J[�Җ�hQQ�G��1��1Ee�mT�/�pc[����ȷ��6��Җ6��s���9��CK[�q��{�{���s��y�ȅ��IW��	���y�K��t�y)�.x�ϧ9�<�֡�t�z��ʡN��DIa0������7�w��{:�𙦁�:  O�,���ܲ�[����Ο�4��]��}��f1��� �Wp��eXQC�{�� ���oQt��_��B	9��[9��);e^ėb1����t���$v*`L(ڥ3v7�?����@/QcHV���xV�A<`n��\(�Κ!�Vp��.��ɍ��C�F;E�LjQ5���&�\�}�d�/�ͥ�������lZU��Gb��l�L��iN�+�&Bm�l&�dLV!I�-�}M��o��/ZPCu;=�3��9���%W5pf���E˗��g��F��uG�oL2�]J�����ZZѸ��-2�.'����j��n��$B*�K��)o��f=%�с����,\�ar���tTD�e#3��� ���LgMS����M�b��e���2v24
�s�Ym�7C�^��Tb�CDD����byy5
�P ��jlW�����N�I�����	��	Ơٰ�i�v�p�S3s��3�:�b�KodHGbs̪Q�7������w(��g�x=�yt���M�^^�+�Z�N(�A�s���m��).��k9�A���,\��w(i�菷�Mo�y����=IO���RNx�Ǒ�MO�@�x��ߔ�h��n��u�9'���7�B^�I�Y07�I%[�t��G�r�g݉T�z�a�D��5W�'bB����j�<`!��O}LC7$(d�Ҭ�;�]s�$o��ܠ9i̿���Ƚ%0m�'����Ϲ��,��bS��`�8Q��Τ�.�ً�L�#"ޓ�����^���f;5�BS4&pt�߮g��K뫳hu��7ِ�h�[O���4e��w��h�nŅ�ު���nl�ppǗE{�@�'��ʕ�@$:�)GcSw<a|h�
�E��$�m�ԦB�T �\�l���7Z,v�pDe4A�y��p{1����ѧ&}��^k��� 1��:s	G=]�l+��1����|>�Y"�%�l���|���)���X�s"4�ǆ+��& �4]A�#�f)E�;�ю�o�I.Op��.�:�\Z�d�l�u��1!,]��͗��P�^_.vj�9���b~��6?��J�Ě�c曷�ۚRX�~[�X�-�8�d�MQLJ���{�|�	u0�QE0����ڞu�^*�?w�n<y�\U���-�x]�MN�4'��.fEk�ʶ��D[./_�DK��w%t�"�=L��^�Xqm	��6$RΉ�m��,����'�z�J��}�-�ȻOu��{�ޞ+�$��������4��%�� Q��y�-F��Uȑ(��b��ښ_��Տ�c�1���\��Ű�E�]��-u��h%�IT,�!���
R
�O
*���A��9{�0�E��t,|�Ǹ�avw�^��5�<�lU,F)"�ŎH�2GuJ�A�Y�5La8p���:�1�P[1ʴ�@B���U4��e"Xa��I)�U^s�7gƗ/�
`$[p�wB�R	C&zڸ����C!��t�w�jdX��x���Xw���A���QpF`,�gӛw�I���A3�D�ZX�)��sL�[�$��|G����]N����ːI��Ëz(m-�9��"�*B�q�Q�����r2%R&��l�TL o�\��15������&�F�-�v��.���
Q\��:1���W���r��t����fANͻ)�����|MC� �n��z(uʨI$Yv�[�_�����X'�i d2�9�X�eP����	.V��^��cUH�c7 ��'/sQ�J"��l-ĸ�K�N$����E«��NE��3���T�5��-��n3��ə����!�����QO.�u�.���|kr�PcJ������^��{Dj~����2�?u ���?"��\'�/��� ��rw�Qe�e�+��Dv4~B�ztК�&�<��)��cݝ�vن�����9D��M�uA�e�K���$��Ie���LXe�g�����1�ۚ�ZY�H�1�~�JX(�Dm���T�rv�+5�hMA�R{�%#|��@a�w
\��8׸/3�o#���3��>zTu2��+3��[+�,�+��g�z����``�Z�aс���v��Mu}�Ȝ)#1T�0l��OBWKt��lS�8�y��.�şW�{�Q����&���>G<zH�3�_�o��LX{�� (�Y&���R��P�LZ�&Z�-�w��F׿����K�c�k2�� �H���K[J)��d�汵%���iE��"a��Q�{ m�&#���F�s�W��,��H�>�C(&��W�]U�6��o��Ƭ��37fs�*d��7��EW��G�I�zV���|Ԝ�4Ʀ��x��WE�5��HD���A:P���͢�H�S��"�-�|�����9��8�=��h��h��9Sk�&)դ�3��D����a�3k�m:դ�I�䉷��L�I�Oష�&mD� �5������� ��s�}�J�k÷x�@DLz��x�V�Z�ֶ%=���G!2,�履��n`֏= �'"��.70' �M��H��"�<�o���h(�����9��	Tb@�B�;
���X��`,;<҉�(Ef�QpCHzB1͍+�Iu^��A���������"δ� �-/i����0@u'�Pm��Ɣ s:�R%>�z212E�l��8�J��>Q�G����I��L�>���(+�q�(w�G`����"�_��I�Z���t?�ӼN�r|����aD�H[g6?1�EZ�}���<d]]���@�h���7�G�N��UU�n����Y�k7H���2ʸw�K_��,~��g5�&:���Q�m��-%�
��a�w�Ҥ���'>@Q���N�6��,�M��:��glO G��x�I3^��}#���4"�S�7��2�����SD� �[ᆨC'�2�ohlOn����PO�:���e�0<���[".�$`���l��nuRG��"��H]	�:�JW�"�=N���i��d��dK7�0OpM5�هHY�tCt����v�%��؅N�1�?U�
��˸������vF��ћ�oJ���7M_��h� J$E���%���t���M#HM�#뵲�b%5������r#�ޕd�Ƣ�Bfa/�/��>o��w{(��C}Tu�j]�Ĩh^��lc�kI�@A�L"H5,����8hR��2��ֵ�[������`լ#�x�J�=�v+-�֤���ޮ�.��όf�����b�'�د�վIh��.�p:�9���/�x4�[�!Ѫ�hC��[Pm^^u���H2�w���2�b��:*+�Z �wk�Ц�"�v��t����j��Škc��/��d������}���6[A��^�J�� {�J��Jg����k7�S*��\y�����������商�Wg��D�u�]Y[5���`]��0�*�q���59��+J~D�XA- Q�N�7ԁ�_<s�$"� �w ��G��A�'�?N���LK��"P	Z"��,'Q���Y���˰���#wκ��$֬ۼ3vw��mӀiжV�.�(@����Z��0������C��'S+�+䓯��
��z��*�k�*	:�岩?��^Eq��~��lT-IM�CEG��P��!�G�
{9a@I�@�Z�fLC���tL�
"Cb�����I��+_����@�%�K_i�NF��4���nf}�<|�H)�q��eu�F�豫Q�	MS����s.T�-)д�r5�"�S�㩃&��*�� �ԝi�}�����	Z��e��-	��i�y�0<ȱ��E�x�I���;bi	^<�|�b��V4�
z�Q�o���ຸ:�h��}�@���=�� `�tO/I����Ž�<�9Ԗސ5���'�v}�p���������T]dN�{��+������ꥷ�V���.�����%���+��++jo[����c����K�e��ʾxU����n���eK��)�[�t+jj(�[��{��ū.��w�\��]���XJ٩�������;j��/]~����Ko��w+WT/Z\�`;f�����nMEm���unM튏.]d�izE�z�{����+�z@�*���Z�|Q��x��h�Z��,r�`PT�t19.]��z�"�r+Ű|E=�N#o�+T�Dį�����-���[^_q��꥔$ D�,�_NI�����/\Y]Q�YY[��n1�=�A���vi]�K%�z�}e�U.ű�b�B�Nȅю(�{犕8��rW/�*��]�x���K?J�K>)����G����UUW��/��"T��ڏ.]��vqM��ZW����"��y��[�ƣ����+����߾�ʓ�# ��ۨ��2)hD��K)qj����WA��o�;��p�U��-wF�{P����+�J��Yq�
���pV٢�P�D�D�*�Uܶ���*iA��w�j/\
�Sף����E��At�J�"}�H�
jNĀ~�M�b��-�}�Җa�5g��vj�s�Wԩζ����U9�����w�v�r�/5�*.\YKC>�rS������(� j0/�]�'�sdI��ꕵ��y5H)��*D����R�bI=���7�>�.]�[��RZ��WIMq�b�V��K���th,�-�:�O�!"�8�dG�U����X!* w+������X��wb^^N�Q�>(�WťeD;����Q��-Ǻ��2�W"���4ݦ�A̩�d:p_O0Dڒ��A��b���xmpY}�����(���M0͍�$�UI�#5�/Yu62Fi�p'J�@�ݠ�m��c:�|�R5���O�y��,�`F����QY��О��)dL�!j%b��N������A��i��%dl���b�[VHz'��t�L��đe�4̱H��k�
8����0��eJ��+�NCiW���R�lq�idIeF�S�����Q��[(�E����
0��A���Mލ3��1m>[��{h��ti5x?�岄%���ɽ���Wx��

}�V8�D��� 7����k�<P�Mc+n}h�EM��2��¾KS�5�q�'�/�3Z�+��6�}�h�"G3�>�P����VG�u��	�щ]aBj ������=�kF~���4���*����׬�$�Yl{F_�[�n+/Z��:u��w�;����Ka�����!%Q\vչ���򘧯_z�e3�����g��3O�Sa����H��8>>�tO����t��T�Wn���7�-Y���g�;��� ��5��i�/!Ʈ�3�f�����=<�,鎯� �-_��[p}�w��&n���(�� n-Uh�íllZ�V����p'�zW���❷仅�<w��C�MK;�C�J�M�=|���Jk���jP�i�7QIU:�O������1���h�ܒ���9sJ
��z3~�b꾑uC����q�
�5A�����Hʵ3j� ����M}N��3X^�מ�e�Q/����')?
�8��Tw�/�'3���t�c����Y���0r��7ՙB/J|��k^o#���y�%sZ�����/&�s��s���7�7��K�D�,�^�`�ĉi:P�����1:��T�y���⺅�Kk��]0qQ�e7#��c�:����[J�m���V\�]�N)�T���4�a����&�&ܛ��>�R��.��<��Du}��س
���_T��,(�����u?o�li��jV���j�9XQ����X�p�G[�.�.Q;�$+Q�YL��
-�A���@"ƨ	޵���Z\Ґ�V \��?�����6�)�Z-��K��T�����8MHy�����ծ���c��\���h=�'̽;��=U�{�l��Z���h��N�S[�=&�=(o���
:�h��[�"\��?bVc=c���6U�,�W�n�Fw�WE|z����G� �Qӊ�U��,0�f�q�}^Ą�$wu��
QW��T�5�WR����k�V�#��3���uwܿf���D��Y���ۥ���4���>z� ��yg@���<���h�*�d,�H��Qը�LH�B�𲦯$$6tƻ8!�0[0��윅|?.������ٳ%wM���n��-��ɛ"�?U}��3��q��^��j�����Țb���+��K�.�<:!2ң,ޞ>
ɯY�Z<R����
� �D�hOH�R�\�Z5�8� N��ĆXww�R���y����>�Eȏh�Zp�DҸ��/��B��b�$�mX� y7��Q��f���[�WZ��IS�!E��
U��h�l\��)e��(��J޳�D����ǁW�[�	�[�|����w#���y�D�Z$���J���\6yL�@B��Sl�H[��3{�	�	�w�A�=J� �eπ)S�]�8��Ft�4����~�ui�P ��L`�0m4+:�5�/��� J_%_/����.Ij	�5%;�Ŀ��G�4g@YG��I������ M$*]qw!u�%�|�s))QJ8�Ӕ�ahU?ȍ]?�>v��Ք��kd��)f�b���Kd)����1�Һ8Wt2�FF� b����#[�;��B�Ock�4)�ҔE���o�jE2�R��H���S�;\5�*�1��sL ��,�ZR%�4Y�,��b����1�����֛���e6�p�w����P�e��=+55���z�,5�)�f-&��|/nMw<o��',�3����d]��͕4f f���l
��ɞX��$�V�Љ�dZd�FV�k�����'G� B�����NӃw3���{�N�d'�4�����ob�.��cy9K4��D/�I�-�P�|S%ߡcR}�;��gz�� �C���:Qv���:Vݪ3�!�RWS���5B����i�f����M�r#-w�Z���7�ݍ���Nֽ�%r(j�Gjx}C~P����`~�'�ϰ�����kڠ����dh�۸�!��7߽��S���W7��@e
�+ю��L�.�rW����ŵK+�(N�7ZA�/֠��g�UPT-(�-�yg�o��-������"�m#��A���]��5WP�K5�r���	����ך�ZM���LU#�13��k�c�<�tQ�?�����s��Y������Gr�,�wt [ՠ�+I{��,	F�΢��47�W+̲I��x��o��AM��7qc2V���*í�6�����L�z�zw}c}[G�M�8��M_j��LL��J*Q�
Q��p��4	q.P�R;���o��Y���U��^��X�{�uǚ#z���So�(Ķ�H�\ðJ��(#���t���T�-U�֌�vC26�nUbY/�Z�)ta�J�4x������Ykےz9�u.c/Q�	u,S=�1�CN^0���:e��t�n(��YL_jb�C�\����+�q�ǡB�d*D�}Yμ�4y-�@�_��j�XJDc{�]�5@{���B�VT�#��v5�10��ZW[���Q�?E�� 6b4�G"ͱY�p\z�F���L��}�N�!��U�Ŭ��J�k�UkP�ϫkڏWb��e[��ΦQ��dX;{%0��<���Ԛ�Q�0�9�$:/�k��#;J�xR�6���Ae�ȕ*�L�gy�vHi&5�M���R��ړ�v����j��Y��c�r��h��:�aP�)�֌Y�X��PQ��v�S�+���;v��XY AnX� ��1�-���8Wl^d0a4��lK�0@ז�8��Wn�т"��z�A��%+�,.Q�)։� �ĕܰ2�XR�� ��.Dg�W�ZD#+Ln�����h�A06』�|��y#r��Ma�ʢ�XR�ثƬ�¢h4J�?ޅ4�S�JF�d�d������q�	����!-�`����_Qo(�=J���L�[�ٴ����x���OL5@�%�zG"c�WGٓ�{{��P:��֩Q$T��ӆWd�2-$S�
�j��Ļ�������F�N�:VN� (z�d�i�bh ��j�]��	7�_�h��	ٮo�K.�.g�*��|J"�����w�t���Ow��z�w��WM#)5zň⾈�8}ojm��6͚�4���5��G�v"�'�`�SVr�kR�4�'��ĉ�ɦ�ى��ˈ:S�HM�Z��-�eu�5��Y���IrO�h�i� ���᳠D�[���QP�\���u+kV�
�P�Қw ̩�̈́�9͞NO����#F��n��_���m���k�p[}%�f�-QD���f�Lj�s�6(V� �iX]�Iu����,�.�B�y�5	(bL}*g)�gV[)��J}�U���k�U�wMXq9�fo �1�"2�TMx�JB�Kqo�p�q��ۮFu�ĸ6��	�N}��c���^u<�
�v}5�CJ��a�N���+u	�m����\�."#48{Dcp���T�y�E�]F��d[{B�����M��&���R�?�9fy=i�,�Q\8gnP�c^tμ+�o����$ں�����:�ċ��(��3?��"�1LJu�I�(YPX�Ƌg�[G��mUfb�=�3'V�(*��R�cwX��*L�;o E9+=�\o��k��U:��ޮ� ;�wr�<�ul�a��>^��yǣ4�i�]�(g?���aQk����B�Y-�K+�l�����:��\g=� �n��2H:+ �m�hl��An�v?��ؙd�5&Չ^V����v�m���W,[�P�x��9+�Z����E�挎m1��E�w��qy���ᵜ�B��wc��-��F�y7�`�L3/Lm���>�a���K�L9��.�Og��78��A]8��i+f0`�cͦ &k5�����#�>g�?��c}�D�Ҝ�*\3�gAw�}�ǧN=N2�UA�`��E�?d=!���J���	�j6�ioen�N�������n;�\�aS��Z�Q`�Z�SM�c����W EY��i>��ܣ�ڒ�C� 4)Tx��Y�n�m����3䢿�oWso����u�# u���F�0�j�;�0�-	�ۅ��лށ��i_��D+ky`P2S���8�yW[w�Sa!y��%K��h���������4�Xw|������e�ԑp�������4���O��1o�0φ�r�N�dCfV�h~�����U�Q�{�UNd�����)`P0�1=}��1xX�	|�x�)�4�|	&�8�g���uL�d���T�|%M'.��ojmko4��r��﷌��
�l��*�G��7�Z�;�w+.�\5�ߌ2Iz��(Gէ�
s5�!�%����S{�	�����S���2YQ^�\ȫ�ey"������"�ӭ�YǼD��Lj��̤0�Mբ�d�s�ƈ}����:��|��AZ�D�?u��+b�yh��~����Wpt�*~V�����q O:�!�wnD�&Ja���'M4j1_�Y��<���J����aD�x�nA�X����%�d�V�eklB���NQ�\L�S]܈c�Y,�c5�%]H[��B#M`G�Q-��s��'�d�'����`1+�#y��r-[���ZG�1��QG�S�%��֭�ukQ9��z��������U2:�֘G4ӻ���P�Js�G�H����^.cӓ�`a#0��0֫��/m�N[�O,��M>y�t:y,��Z�X�ۺ����z�z�AS$�'XL�:�^x���h�]f[U\���uʑ`��B�*�<ڈ�n,}TНm��X�S�,%������hHȩ�6��w+~��I�Oj=F��M������.�FHtf�e��{:ٻ���ѴL�ڱ�H��*]
�>�hWd}�=��(�A2kݺ��OT�m�*�p�6y�B$�xUŲ����|�D�4&��sk�i�L�-��d�;[�7M�}z���V�A[�Mdh��wl��B�bO륒Z`�L��g,���b�J=밵�oN?M@=n�%�H���էmn���ڻ�拙>�k�/*(�[ZPXP-���
U�> �%̕�Х�ڋD�\wyXmV��lO5�f�>�7��%��#V�T�Θ�>8C��u����*Q�Mğ�� a���v6Ό�a�a��f{�܌�(@����j���%|�[�[b��l�� �����k���jO�rE]=�4���[�K�6�qsG]�mg�@|�+�ln�����aE��.Q2&j����[�j�ހ<0�a�%
\�)�D����Hi��Rf��A�K�Rn�]�tO-�<�yn�Jv�ƶ�
Z�#����*��X�i����`��j�[��i,F)�髡�)\,�$� ��N�8n�e���S�����`Uݖ%�#ZW�O=�XK� �殶F]�E(����Έ/o���CB~5�����r8�o=�Zj�/6o�N��v���fa��a�13�i���F��76����ah��Ӕ5M�oi�p�}���xZ�ѩ�s-��ښ��a%VH3���\~,\��rsTS5��h	��hܹ)���e7�*��e�_�GG����݂ GR��EU�M��u��j$O��`Z�v�7���k����jY��aܻ����=��&x��i~^��e�7�����Z�/�.��F�w`hi������c�6������F�}���v�7%��c��"��4�\ʱC�Qs_�P��4+��F�����W Gi��v�m�����nm�#_f�)X�K泦�"�k\�޼�����}ḍJ�Z���x��n7�
V}�K����#H�ݾ�N=�I�uj�@*������X�3ub�26	���I�@52��m��sW'��L���O�K�T�2��E��ZU��F.�!�r�Y">u@�'@$F�\z9�垦.%[]���h�qR��X�n}�άXܛ	 MI`s��S<\��+�$�Lu�;.ǆ�)=]{��H3�,Lz���# ����S9ï�hi�\�A[D�H�+j�+��Os�.�R�jw�ʺz�P3/�q�U�e�%�{͂"w�$$P�y}@-�o��XZˍ��+@���e��Vg���VL0��=�&>E-i6<\�*�R�pyv��X{���x����2�9�
�ni\�ﴅR��o�B��.Q�.Q4���(o�*1���p�`Z��@��W��z�F���˭����2i����<�Zg�F�[�J]˅Ż-�hZ�~�V�z�2�X��fr�4���F�W�^N��w�:�jc�lu��2o}�����ʧ����[���f�Jۧ�ƞoE�qԶW�<C��.O��*iodR�U19�M����ZZS����*߳)r���)��\��t;G��͌3S���k��1x{)��f��u�G�U��}�E�w�i7�����#�\�^�,�R���/ {n��� ��2�3*��m�V�7	�\ق��4�����.�ƩS��Z[��5���`�$��7�s��Fk&����k{ZtCy\/��
6������J�)�v	���l�#qf���˾���)��}B#l(�V��v�JJ9�j����R����Ry=ó���F�č}�@�����f�ۙ�" )�VVI�K�f���e���>�	�Pޢ��a�܅O�����P#����`���fwV���߮VW'�x��|��lr�4ut��na�6��
0z��(�r�7��Q5����Jc�����s�J���z3~����f�P��(�Xᘀ�6T�zj�j*ՅYܤXV��]Q�����@���R�-(�����la:v�}nuI�( -u�Ƀ�e}u
�W���gu�%�TxP�Ms�y���z��˻��iĕ�Kl�8j/*7J߀�bD1q�QU�X���I�u5�v�;�):�{l�����8���dCC:m ��1it;�M��a%�^��R;�(�m�X���1��+T����v�U����p��f���qa_]d���1�G���P���¸���s ��U}t�DF��.VMݪ�"^��f��$>iv,�4��� /����:y���5��jл�Eƭ���U�ӵ���sQ�y
rv+�A���9�	���r|���2-m�z�����!�J!8�_wG�����P�'�.7FZz����ф�i\v`Tn�Y=`=0�c�O(^UR3g��N֖ܰ ��X*V��G�J�
嗯'y����N���8�Y]o<��z�[C-jkTbN��׻��-.)t
��s��E�9s�b�/.�r��M�A/Ƈf]�߇�����k��]���e�KK�[������Q�W���e������uݏ�&��S���%���Uw��j�Ñ�G/2OF=2$+G�����'2J	+M�O-s���4M	���k�8ҷ���F�/}�6�!j�4�F�ϛ`t��s7�͚u�F3���7Z�r���w��ϴ����U]�w��|��Gߐ���M�$i[i����&��~~o�6�Dj�@$c�ߥ��o������]�X�ϟu��&���sL��FJ��)����ģ����8�#�4�Z"�؝�9��}���9����.]$zi9�Qr���[��F2�:�cb������ѻ��A��I��LQÇ�f+�K���og��럭�)'}|c��R~o�l��>���=�,*K�����z#����┆̮]�p�ҏGh�u*
]��:��Q���Y�Z����C���2�]�/ݳRK�zR�����g��=<�$������]�DA��Nc��Iq�����J�͉E��s���7�������D���4��4��)����s�eTǅE��b�+fvkL�*�'�A���P���d����u`V�y�٩��L�XY_��I�,��jgs{�H��_i4�Ο_���<w�BO�7T�W��_QJ��C)9�[��ЩW���V�v>��C�^g+���a}ɶ'!�j�ȫcZ��T
���	�\�!���Q�%��D&�V�����ᠧ�s���(��.(�K}9�V��;P/|#νuŊ�H��������.(�cF�3%]�PoW�TsfG�g�s̉*��]�=�M�� E8@�TE��م���7Z��dނ"*־H�m5�L�`Js��م��_8gAqTAd*�4�	������E�B^��$k�����Kq��+DڐH�:�zQIE�$���stR��ǭfM� �������P�Z<w8���n�O��F7%�<U�(d)U�J�C�ߔ�_{ϊ��C���Zh�hq��+�/��]����-^��� �S�&�uXnq�NMTzSd�l�OD_�����i��ЋU���J.q�M���J�/	;Q��4��}��_���+�������|Ʈ�=]�ߢ���W�B?P<w^������yE%��'�L��;�?"pތ����I�O��J@�G�#p��	���9�r��hl	v���o!�_4��)�k�M��&l�C��@y�8�҄(
 ��/4~�Z��@ҊZǪ���M
6�Q��)�[�FT�؅ n��LAb���u��q�CŢ�0,Kg�@#��Ȅ���Ťf���
ӡ�&1�'�{���3H��͈�TZ�.��4����PĢb��&+,�7�I.��S��C��R|}"_臏���s�X�	���ӛ�Js�1:ݩ�B���2W�Pz�u �Ta��_QA���8%�;���K��RH8���@��D%��4�u5A#�:���KW��)d3Q�f�5�Z�L��?�Mm_\l�C���5K�MM�DD��EUpwhQ��i�G�4=�S-Q=��L�槚J�ՄmC���n[\|��]��B���������֢�ih�j�I��u�Xw�;}��ȧ5��Pq��=]�r�����}_�X�?
�J��OK��<7��$[�6�� pkir�/�������hc��`�:��%s
�鿢��+�ߛ�{��_�����3�2�u�*	RQ��vCD�)�d����B���W���)d!�tT���/U���\DSXR��t��@4,���3� �p~���xt��4HF�4�.���A7�-*$b�j�Ǡ��η�1��&���F\��5���yMg�ICg��Y�BgRK�5�q�;,Y�t��H�����S)t㜢4������2�*ݐM��MK��[�`��s�yݴ���ԝ����먌ݢ%E#3v/����u�_�{�wb��]_[�pq��ܺr������b0P�E%�/��^��EL/�*�(�ڪŵ���-Lu]\WWq�bv.Jq^�r�BP	KW(%)>�.�_\�������v�������45��uWWW,_�be�*Df*��W,t�j5�t�M������=-K-�o����@�X�*���.9Q�&������.I#���E��b��/���9�9W��7�7���8��Lv��?����l��70�Xݨ��T�z��2��,t-����`V;�9WB��\�_���'?C�(�|#)�*5� ��N�ip"�*VV�7x����g�a�\�/��`Nij,�P�X�`:}�]�}�]�t,2<�vqM���!SL��+؄�;EF�۵�Wϧ����
�<�l.���]m�ɠ�F`��W���ߔ�[)m�@�΃�HaIj��c�g4����'�3Go��zUS��U\Eމ4�;G垃-�8�y���isE��D,mL�:�B��,ZP�G~L5�+V��PW��~ъ;���nq��/M���ޱh��-_Ⴂ4��'�֞ds��α�$��L���NjH�8c��hZ�z!E�c49�>�&�&�hA��0j����.E$7����7*��OY��F-��&�~��՞��|�*:}��������*���}�F��t��4��V�8	/8�pY�0�������/ˋ��uMM�s�"R������ ��ȲoJ�*Q=MI���-��4º�i����jF�\s����Dʨu�*�:	� ���*L��Ξ^����24!8�M�������)d��"w�T��DT�,0$�T 72�ӑ���&~��j��cE�M���~q]}�I��D�%O9� ��I��B.Se�}���߈�7���h�@�LW!��ꀰD|�,ȶ��P^�
�+Q�E���.���qy4o��I�����ɠ�QgX����#�D��}Bs5�%�E���I�����%v#_TM2�A:��3X�ʘ���KE�D����,e�np��v4&���������*P�o�uw~�]ѩ��;׵�<���Y���F1��U�`�(�iM��´���P�D�J֑&L8��R����w׉֍Ԩ*�w�#�i%0�61��4������V����Y*9�$���y�����ń�R��V�Q7�zt����N�T��_�yڰ)�'��J�+Z�Jw�ڂ�y�����~u�>w������4�t����	/X�b�Ģ≖k����B�y��e^,u�GL�ZT&N,��O���Z��xV7�$������/j`J��N��z�m�	fe4�R��^M~�|��F�o0]i�Ovt��������R:�{52��k��@	�N���b�Xh�m��zDs �r���t�",r��.�(�G�*8�4��������f$5������p�0�)���]-ڰ��3&Z;�� ��v�$��jH�v ��>�����{ے7�ݠ&�;*W4P�KoR�Z�]��u%��d`Iiro��W�Ҳ��R&��Q�N��ti�E��NNw!%��~������Q��@?�v�븵w ����f��Wy񢭯��zq�����w����W~�7,�����M��x^a��_R47z���f�� �����+��e��/ʿ��������we�r�_+��-��9'�EZ� :7�"�$uw)"ai`眲i��˪��
�-W�V�7,�w���F��-,���(�ιB���7��K!0hj���,����H(�>�4�gv�R��� ���
Y���o+�м�Nt�tv�|���ր�9����&)J��p�-�,G1$B
�(��BK�»q3�m���YDQ
?ǦXY���SA�+-J^��Dfc��~KF��`]PH'$��-L)	n*��%1[+zOǥ��,ŗ��C���K�)ļ�ogI��<f�VjD��:w��دR46-�@�<�I��TJ�YCaNB����ె\	U�P x��M�m��i��J����9�ӭ�U>jC�SgŸv*���2u�T{zL	b�VZt�}=��&g*�ՁS5�e����s`���F�b:�M�˼W�Xu��ZӖ1�κ�jH��L�*�p��RKjE�5�D�IHt�|���H����X�N�ޘ��"�p^r����5r�x{S�%_��I�a��Мs�x ��������V��
�t��+��� &�m�}�]�]�V�Z�e���Hc2ު�\��2�y.͟v�^xe�~e�~e��69�ymۋ�7F����1]��Gڼ.(�.h���W�F;�P�nL��#���{c`�Ð�/�E�x7�oecԿ��s�)U!J��x�ƽq��i}��#Mt��ӱ�&�4�x��oLw��5/A���˾�!��1o�X��V �"W���w�~GiYW��S��H�j����A}�
wc��x5*h��@�=�7�*�O�c��H�\�g��Z�
�@�j���֥3�`uK�.>��:#1}s�I�i^t�%y-���1{/�넕���(]�F
i%As�Zj��>���&*�a��^7�w�C�Y2������A�����)|�����ڶ���=]5"VZ�;7� l��͙3���(:'���9�K�3�D�λr��M�}nq��P(���N�[���SB�m���3��q�,r���]����~�~2��pؒM���'�p#{�=����'��ʿM�,���8�w�RO9}X�t�N�uj&�	��,��J�t+��=��?����ds�����vv{�v�<���b��#y����%�������J<S�y�ą��^�s��*	�߻��8�Jۂ�/C���+K����5b��p��=DO+��3��K�$�^f�;�.=�؋��J�s��-��{�ݯ��.��r*��5��fs/ȡ��=�7{�_����8��?��p?�?N���������-����fz�����$��r�J�?Ŀ��q�ퟣ�S������<����V1:�:����N�G�j�������퍴�L8Ƒ�� �	�8D�pQ�#p�vL��� Q�ل��:�^�v�$N�Z���nZ�[(x�l ώ����^�X�C)ҧ�����Qe#����۪�޺��� S���� �����먷�}aS����)mmW��bOR{���o5��Mj�$�����ok���H[��ڎ7�.�i`^u��s|ֿe�(t��=_?��?k0�������g����P~��@�_l8�냃���a>j�惆y�a�k�w杆y;�g�5���;�����W�}'3]ʛ3k0��5��5�yh���CC��=e4&wS^��˕K�2���Oqw��O<�8ub���0(o�<��݉�� ܩ��E�p�?|���y���ݦ�R�G�o1�G�Փx!L����/��\�kp�5��N�n0�>޿u����"|���yO�}b0átr�N���;r��9�ͅ���wo�S���>��^�����w�nN��o��L��_�xs��s�������wH�{S��"����ѾSyW�g�;ϭ���j�K�;�D�u��&�₟��r������[�&�OH�IzO�+Bu���GfUd�"����檁\���3�T�:r*j�YO�������Bm����"��������j c�W�w2#��?����t�s�e���=��9��TWT?�)�-���{� �B<��=M�w
�s�U�P�9�k�g����s������ZD�jzz��KO?=���g���f͚]���<���Q�l�|v�����4�(e.�������;9Koyߩ��#�~�w�����_~�P��Lg�����=��~�̽��e�c��3�t4����q˝�u������8��
+����TV��k�S>9�?ս�t^#�{�:z:d������3Y=ߢXKcO{�s�y���V��J�-�Lvop��|��A��b�Z��K��;�Hi%UI�w������4��N�_��j�֣lYۯ��7�[�s�����J�Ou|����ݶ�Ǵڔ{�V�>I�8M��+�1��0m	�4���l>���Ђ���CC�#���Ty=K�/mv.�W3��;��?���x��<"�opK
���yS��_��ő���ʚ��ˑ��6�=C�?�����M�_�w֏׫�٭���D{�]����x{S+QSk{�ڛg7u�	|�����Y���o6f���5�m����ie�:�d��
�w��nJ�PDɶ��7&-�D)a��V�ᨩ��v��M\�kT�Ak罉e�|�����_9>#뺬�Y���M��SU/��o�嫯����"�fT��H�Pq\]<�ݡٳgGg���3.�>��C8�_��3��l�߹
��c/㚐�,�`�o���!=�1G�l��|���_��Qp;d�WПF��	a쏫�]V�_�l�3>3+41tM�лB����F�ڰS��Бl\K�d7�[�I�ht9�v�
*n]:+ٸ�)hmL�:�:) ���N�I���Z�r뎵��ړ��vn�X/�U����ڈPn���)��6�t-��pҺ��k�'��U;ښ(�8�ƱО�)v�-��g�Q��ڊ�-�i����]��Qm�a�-"O�D���k�qSs��#�l���l�w����b������8������5���<��5��.O!�&�m���a��f��:�=���`�n��?��_�d>�N7,O��k6�X��dr9�t�kr��X��d��9]����f2��)���z$~�!;����_��O���ok��O��c�'�2R������o���7�=��� ������{��ua�R|�I\�쀿���T>�^ϵy���9d������s�?��*��wP҅?�{������	�1�i�q�?��/��@�SAK���k�xn��6����{����ߜm�OW�ӆ�Q~�i��
��"y�i����!gҌ�W�*��4Z��r��v�H_n�_����@H�I���G��4�7��s}��&]���t���G>���~��o��g���{����Q��W�g�Z=;��y������νN�2��|ĳOP�3��k�gg���}f8|TX��'qxvw��vU�:�!s�k��t�g�d��Y2T��d;së=;�(5��g��lm�]X�g�zo���q�_�w���_�_�O�����j�L�C��\ٛ����v�����E����-���<:�*�_�������p����س����w�κ:%���&���F~n�ǞM�_�گ�����Ձ��;n?�g��8~�Q�x�aǮ�Q���f���w��8~�Q�{�a�,q P����D��1��MF�"�݁����O��ϲ�l��{�>���'D����~_t��
��yGȨ��d��#{q�O����'0I�ٰýհ�����R�o�gS�B\>��5T�q��f�yv���p.=_�� d�]�q�>�z2d�]�<������vN�쳬�gC/�쳬Jg��O���	�gM���]R���{I�>�Z�ӫ����}ֵ6l�uu���Ϻ6�$l�u}%l�u�^���0��;���6%،49M����ٗ3�љѬ<��;�������#k���իӤ�x�eihNƻ�=���T�`n�ܢ���=PC�EG��{::6P���o��k
����YP<��ԉ��̒\74,�����T^�q1�V��֊�K��-��-�a��l���Ew.�X�t�c�򩓻�r�HMN�h���ީ\�P��z�����m�����!����aCs"����������]�y���v΍�Gߣ:��ɦ�7�@T�^�Ǜ85��	�������iib}'�2���
�͍���^��y3~�����M9���t���7Z��t"?����?���?�&�	��r���_�;���;�;��H��º	:>��O8�7��yBH?�~�]iٝ��1�<�5��0��
��J�AS�������f�K�9�E��*��ޭ�f�+��i����k$��S"4���{���D�����F�q �	��������-�i��8���m��އ(�R��/�#W@_�rl���
j��OKK{O�5��-��44�)������?�6d�B�'����{3�û2�=�Ѐ*���Ϣ��TFX��o�@�AS�75�*�i0����ϚH��O�0r?���Y���|��c���a>d��}�y�a�3�;΍M֧h��g5u�mD���26Y��Y���L��$�1r�>�y��a�c�\g�����L�Mq���u��o��Q�oG\"��]ȳD���O�C}����A��zz*�s���u����)�A�6E���|�TB�N���Nq�0���J�:O��,_�o�24���-2'd�r�L�T��T�ŏt��Y�fa�I������|�_���78���Q�Î����[��3�gӹ#��CCvX�S���Z��-�rF��9}'s���ܦH�{
}Su�Ԟ�+���cUJ&%I�Υr�E>&�����gd����fWMHm����̂9�E���Y�5��}*��[z:�w����:�z���|���S�F�sQ���Wɺ.�S�l��d>^r��|�DX�kX:��~��@8S��P@��� �h�O/V�?�c�No��[,����y�R��@��$�Q)�'�\���"����Nޣ4E��������=0S�=2e~F��%:����_��Qp�7ܯ�{\��x��/�ɟ)��1�{`�>4y��������{T:#�{`씎A�C���]c��{��t��e�"z,kty��~�W�Ɵ)��TG�� �d����w]�4^�0�Ly�}ʇ�����{0?ktyІ%�g����౴F�׳)�}Lo��#�����7y���{@�e,��W�M�c�Uc��8v���=r�16y���16y�cD�y-��N��m~��{�=���8���E�C󺴼G�ggy��y�������=4oM�{�xv�٥�_��e���=�=;�(�<;�|%m�X�ggy�^�~y�{d�2�р�G��Gc��c�^����~x%?ѹ`���{:y��J/�Q��	#?��=t�iy��@�K��{�_��,�{�y�G���ey�U��W����(��cy�҉�,���	��"��G�Q�Ho[ �U�����I#<�7?h������.�>���b�?l؃����?� �c��y�?��:d�g��#d�ًB����!>��CV�C���B~�|Ȓ�}vqG�˧�Cօ�0Ft���]!�l�	�ú��v����}���}�q$��_ˇ�c�>�8���+d�ud��ȇ��ȋ�7l�M�3�Wˋ�	�W�����~C��'�ܟ�K|=a�l����������@���g#��F~�����w08+y�ʋh>����̏��H�e).�A�Qb#c��*;�7Cx���!��2�s9>��l�GR�?��/m��Υo����+��ޔ�p�;���w����!B�f+?o����l~L��R�����?Z����"��=�P��������q /�y�G�Ok���7�������v\Y�O�����pq$���I˩`��^'�������i�W�z����K�Q������!�E�JAG���g)|��	��.��y�?�rR�"����)r�0����w���U>]��K.�_#�M�*���ߧw>�����K��gST�����A�,zЧ���y��
{=���+�����ć���}:��v�CF��(O#���r̫����/�>�V�����n�ׂY�F>%(��Hv7um`@��Xcss[�CtdP�*��tA����X���HĔxJwwg�eR�W��5�t�ng�y�w9��x����sDcI�7�F�ބ<�/����W�KȩrRea�Ʉ����]4���b�֫X��*��麊e`�W1N/�4Ylě&�MW�L�f�i2�r��l��1r�]�22��	l�4�켊ef��	o7�4�����^��o�IT#S�����c`����y�a�b�7�^��e�[�ü�0��J�\n�Ks�0�f�0���CC��)d2�=m�#c������t��<ؑdy0g\�,O��0�;�l`�5�O�L<�2n�	w�`W���-Wǔ{�.xm���,�W�er�NNt�Nc�����C�!�>C� ^�7Ůey�/8?f~�i���<a��$�]ʧ+�D��⭓x����K���{�r�{7}k��o�TN�M������fջ���Գߧ�ܐ�|�q�s���x�/��!�p�������q��9.rQ���~���Џ�֨�"
?[�&����u��]�EMw�m
I��$J�H�藧G�����?�����t�˗�S)�T	�á�k#Nߩ	���G��h�$��ϧF0}6ҳ����<?F\����>y���(m�5��f���=;١�Vv�	 w�k�ﶴ��ݮ�d�,%���qy���-s�&��b�H�Ҟ�+n�_M��GF�v]~9��Y�ф�*4�ʗ}��s	�+d<X���~�J�:����M)`������,�"#UJ/#��*�����2R��g�������ϔ�z8 �4�_�wG�ֽ.�c�.Cܜc�oD��Hێ$G���0m��9����xZ9*�y[�
�����3�0S�PX��(��D��ǩ��S�'��0ܯ�Q�:f����º]J�]�$G�u�2���`�L9*��SD�馓���G�3���Ǎ��:��8�/F�����:~t9*�Ə�5�_P�
���8gG���X�2it9*����@����f�.G>٪ >L:9*�ߎ.G:p�0�L9*�;�������Q1_xt9*Џ��b^�_P�
|��W���2娰�>x��?ݟ�rT�79���&G����Q��X�v�M�*����Q��flrT�_369�\�P�����prT�.e��`�=��X9�ҀU�'ŽN�+��/��rT�{v��s���4�U�Q�yv�����h��s-G�ó���k9�U��g����Q���,G���yEi��<�J�y��߳��A�~y�Q���r>�r4;ܷz�3�[���A��v��������|:+���@~��I'G��O�Q齻'G�r;��~Lϲ�/>��Z��Y�jF ���M9%�Q���c9����,��J'�t�a��d�S��o4���.��{z:P����;�.��Ov�ڰ�!�h�'��rO�;���B~����:�<��ywg�˫��B���Ɛ�?�~_�>��3:��0N�i�I�>���}^��!���?B�y����rR���w����]A�>�*��w�����o���vz������kc��=f~����f���}��7a�|o0��_�����3l��v���|������|oi�}����s@pz>�8�{��]A��2�HY�cESFhT�+9���\!,���?͠r�j�ݨ�T+~[^�N�+����Ip��l *_��τ�_���{M�>W�$XZ��^rhmz����d��4�_�.m������\��z3~���>~���_�@>���7Z��H6?���<���-��T6��z.���%-��]v2Xo�B�����}�{$��K��d����>[�ҴƉ��Ib�v|=c���`���U(��ڰӂl�p�����s900�4��C���Og�6=�d?�)t!=3�8���s@g�����L܇��}�]�Ӎג1z�O7a�PoЍ/�sؠ�~��kr_�����_^��	z�t!��6t�'��fЅ���|�y<�_����?E�=��O�Ka�Q?��<�;�;jЍ���rk��sA-'v���K�0�-/�rb���۵�XZ�.-%��������N��XzI1SV,��^^�L&��O�E���ȋ=%�b�E^�ȋ�����;�ȋ=/�bg�b]j��}^��.\ź�Щ!/�y5ˋe]��b�W��X����\��b�W���T�i�xO��j��{ʐ{3��i�Vü�0o4�I��n���j�\o���"�\f�K�(W�㡡�г����i�7F���\���y�I�s��ʕK#W��_,�;}�`tڤ�h�u*_��I�y�I����M���w�-E�o�/O�p��6͗��-2Y�O�5��#��[�J̸D6�"��ېM��݆U< ��x@��VdȎ���|U���s�Χ�f�:�p��(��b�s3��)��@�s$��4i��4�'�o�r�0Ӟ�B�ki-�C�ֵS���0rg�f\��ZȽo@�`y��8^�u��z+Ȏ����q�o`釫v�\��Q�9�����ܯ�e	��J�F#ܻ�	F���F�hp��F���4FD��6�?�C�z��sv�rhO^9��Q�Жv6Ż!A�����:�vV&b\�0�jM�D��7Jnmfb�Ll��}'%����->�7d<Z��Ț�V�}$ kvtY�.�5+Y3�'M'kvXd͠�Ք5�� ��>�^��B@���A��)C֬��K�X���+o�5����w$y�"o6u�y�+�foy3t��fGS����/_y����`��Y���(��DSER��	�����W����ɛ7��ɛa�.'Cty3��]����gʛa���8ƭI�k��1ț�6�:~ty3�3�3�1�����1ț���9!�_P����8nfj|��0Vs�G�7�xu���4�Ly3�_���s�� o������@������ o�|����@C���@|Ay3�/�]�{���c�7���ț���Y�;�&ovv��fS&�M�l��͎�Q�lh��f[hSQ>y�#��|��7+țu��~�ɏ��م���TO~�km�ggy����O�=;�&�ți���7s=;�8����?��y9�7;�7;��yEyʳ��w8 o���Z�,��/y���<�����R8\���J/o��Gǧ�C˛M5�O'o��Y���N�3��O:y�c��T����u���[�y3�ӳ,ovԋ��͎z�b�7��c ��y�#^|,ov$۟%�ɛ��א{2���y�̭�=���Z�̬?țțy�y�#��fz�hy3���fg�f��f��hy3����y�y��y3��f�9�c!����C���w����d���!�\�f�࿇�?k��!����}Nx]8p���	o
�焷��s�;!o&�!_���{��9ݦ0�ݵ|ٶ����2��G�<G�Ӱ}���a��t�<g���'e��9v��d��̰��d�爕�9��1�a�#>����ȣm��+�}���7�������M�P�_!|�'ϯS'���zI����lpͮ�����u���xO�RiE��xnᜀ�ߜ����7�7��_�>W�yOz��s�a9���7Z�Om������'�К�'���9xB�\���.�Q�4(���˕zAQ������O�ߝ&O�4���[�#�;|�҃: ��4���o���\���ۺ O��ߡ����0끀.@���aS�� �|�O�C^�r�O�B��K�{%?�x��C����O�c�|hM�C>n�u������.��>`GG�~u�/��@�C�!m��_�>���ҳZƢ�9��\�����g��Q�T�D�����)hJ�̝���X�;��t|5S���EЗ��_����~�I��{���ɬ_0k2��eOf���,#7e2���Nfٸ��S͝�j3&�l\�d�R˟̲pQ�i��M�C�d��3�3#㖇�1�^60�s�a�1�Y��1��_��g���o���c���Kc�A{jh蹟CO!����h/=F�?�҇i�pF�A�8�X�,9,Qr9:Ly����]�_���������58�e}��d��wv�ǅ�.���N����>��'���[�R~2���X�������w��Ө����ള�-�:��|~��D�'sݾS�!��ƮA��$'��������X^(��{vfE�V��o ?u�����]�du=�_�58���e�����������#��SY��?�Y� ���@f�}����78��E9��O��R��A�y6�l(s��p^"'���E\�������G7Q����G�Hc�9kjhͼ6���V������:_/���y}&s̓��2�P~�q��7��/y�\)�#O�)?���eh��(��OU��h}6��I�U!�5C�R"�w�j��p����5��v�v3�J�����M���������5����]�!j������Jvp�K����?0���?��;+��� dC9�H��~�Y*��c���3H��򂴣�g��ߔ߾kp���W_Umz�i�ډR>����W�/g�����g���~'�'W�;!��{��/��<���G�� �O�ǡo��?��ޏ��}l0th�@Y�cT�էNQyr���4��������W9��=Z��1�o?��1��'h,���Tz14I���&��Z2?�B���>	�2�s���Jf��Ϣ�3�4�_U�T���vm�vF=o&�έc���%�߃��H�{G)/�[�NKwmc��Tbx�=]n7VoH�%h��u��/��b-�=����pzoH�>�K$Ӈ��k���F"�&��c������B���!%�L��ΎX"Ѹ�c�7�l����h�T�f�=���̩R���!�y{Zζ��E_n�������A��'d<�y�7��B��F�M��̜�r��D��+r��J/7{!��f[[n�R0ˇ�htr��n���g�/�����]fr�{eψ�ˑ�I.:W`:q��\��C.]<�\,�|@��e�È�g�M�øW�]Q6�G�N��a<$s����0����=`������솙w;�\,��c���g��b~�"�k��N.k�;�X���c�a�P9�i���b1v��].V���X&�����I��ǥ�w��c��p1^��ߔP�?S.V���A.|�c���s���bA�e�ϔ��,kO:�\,�ό.��[�/(�s��ɣ�ł�WI�VGR��b�o,r�G&�M.ֽflr�YS�&�?elr�}��\�w�M.�=��\lm��\,���bk��7�Ar�n@.�X@���;h�X7 [�a���|��ϳs�:��Y.6��Ɋ>� c�gg�X}��b/xv�Adߢ�b3=;��fyv^Q�=;�|<;���y�����엇\�^�?-���Zn�a��nO��(8��@|�>�\l��N.֧����
�焑�tr����\l�����^0쐋E�1=�r��^|,���{Y.�	�\j��/ذ�<���c�X�Iz���@|������ݻ��Oܟ�/2�j�;���-�����7�����Oi�: �{x\�ӎ?^!����lO��z�ANk�����L{i��ZN��=���S���;��u��n5��F����.d��C��jCN7���!��ǋƅ4�����:�q!���������5���	���r��6ʃ����}�>9l��O���ya��p%<�r!�k�@n�<�o�Y����d��3��<p���~0�����F����ߵ�����9ê0�_ؖ/��@��¶|A8Ö/�ʰ��Ͱ�n$;βt)����%����?x����ro���$:���E%o�|n[gEw��2P�;*�����.h;���y.�πRΊ:��m-��z��$m��ɒ}A�/��$YKM5�[+2��LP������.!
���%�9�J�%s�W���)���?K��p�C��?O\Ot�7�y��?�\Ϗ)���B�9M�o��?�]�'��[)�9�a���I�|������8�[ �"#�$�?��3�a^��.
r�؍a�>Y�`��5����3"����KW�4��4��=�$�l��,~��I�;H�|���;L�M��+o�(�լ�����رcE�݋~��ƌ�an�������4�=d<�0߻F}=J���w����a�FΤ�[R��މ�}�r�Y��=�?s�a����C�E��}	�[f6����{�{�(�i.Ȩ{�_�3�D���<��#>�]�2��;hn`d�/�i<�Ж=�X��c��~�{�3ӧ��q�#~yq�����!D]��W���o:�{,�4��ɱ�>�| _q�򂄇���G|�Wd/[��o��ȸ����!��X&�O�}�Q�b�!���@W��GX��E���#��:`�}Ŀ[���G�����g��%mU��ؽ)�{T}��'�G�a�y���d�v��e�>�����AF;k��x��R����l��X⻒�n�|�WY�?~n$c�V���g�ú/�̔aSk�i�kP�p�iI[K|Yb]Z��&�յͩ?5ǰ��>ul��~$(Ь�z弓a]�[�mm��5�t�:-M��u��ްt���-��a����Z���JUD��t���Hp�ww�%c�Z[61�dk��TӘlux붬1��I�mP�S�k��j���4{$�%�����&�XKok��!Y�l�َ���_-�� �=(�]!�&���y&��ނ7-�[�E}޴�oǛ�xӂ�oZ���~7�X�g��x�L�Ϛ����3Y~<g&ˏO����3cu�L�'wg�<���,O�7��V�g2�jt&˗���r�i�-Û�򙌵�h&�讜���3c�o"B��g�D��ƛ��5xqӌ7-ҭxAӎ7]x1�ě�^��]7�=ז�N#�^�C��=>0z�E��R�5�y��5̹�9�0gf�0��7�5�gs�a>n���#���a>`���=���0�0���ü�0��.��j���U���0W�86��z��l��<���&���"�߾�1�F���:x�2�g�����wl� tp�G'��8�0�Τ�9���I���Tz�靣ܮ#���^z�a�����<1X�����g {�g�G�j؁�8�L垩�{3���{�����g�=����R�?j�1��u��}�?�e��o��p���ô�j�)�@ػ w.���04���W�wQT�g��Ku�.o~�.o���1��^����j��S_�s��>=4�OΟ�����-߼��lߌ]�5�65�f5T�Jj�jj�%�4�m�EQ/���ˏVn���~�'گܺ��[H��k�G�}'QwՂM�[�1������Yd'2��ѥ��#���,��_>��.��&z��М�l7=�������C��Y�SH�<?4d��o`?���sھ�X�,m��������t�R[��XS,u��Ot����~��tʮ�4�:ۏ��mQ��E:�W)��t����i���"�,�z�]�O��)���W8M;�E�}B��bF��'���ii~�5�悢ԺD>�*�1��p��[%��S��-O�G?��e�"��N=Aat�z���B�������U��<&M�ԟ�(���8������~x|Wd��(��5�F{I�ܾSO�E���7#����'���֙�o)�w0j�1�G}Q�� �#����r�5����[,wG�p2ϙ�&�fR|����~�[��/�<���8�!{���4�m�g�ѻ��=��%�1z��=虲��C}�'Y��I��w�uu3��
�A�_&�*��٫���������%��9���g���L�<і��vz6ѳ��C���'�3����n���'��9���\�x��;h�^E��wu�U��j�u�<��`�g�G���(pǩ=�����z��wi7���v��A�I�5�&�磾_��<����	ŵ]�ő�]�_|��n�������'�󊍫^�
��mF���v�?���e�������T��S;��h;�l�����Y�ϵ��vg����s9�F�ԩ�����$I�$�����,�M���h�Y*�6�s���6Jcl�ԋ�7"���wc>A�S���#�k�h��F�T�͘�ˌ��\k����J���⏳���2�iѠT�ՠ?�L����U�f��Z�)?����䛢٤Mn��pGl��X�L��ӏ2��Ի��>�a��ʴ��e ~��5��f�c��Ln`�d�:�/]�����Q;�����;���&�vOt��w���&m�������rL�r����	k�I�,MZ�<?�����<;m��� 8j�0�>K��g���^ 9�m�� �.�^_h�ȔN삯oa�Ѓ��6Zh�ʲ���'�X��`����~$
ڂ���>G[�g�P��at,����X�"��K��_��4�C�9t� �l�����N����ëz��4�µ\0t6H}�#�dp�A;ѷ<r��ӑG�z��>�mU�Y{9Lm������&�s�`����a�Cm���h��~��j :&\��i��5D�4�y=��|\i��`���J#r�͌~m�0�����<�p�1���)=���T�C�~�ܧ���b��)���
�w5�}
nR�%4���<����_��:}� ����ź;��RX]����b�<�b��6D��2»�G(�K��~���2g�_S�<��w���{)n��1�/�(�^n�����ib��m��g���G�N��;X����˾�j�S�~��Ծ"#]��6#����FZX����FB�[]d���J��^���MY�����v�@��yϧzU�A(�{dnϤ���L����H����@>�%�
�tK����}�e���N�k-��g�^ݞ\l�齔��B{ܔ��)~�4P\�i�m?����Q�Ǣc(��`�S{�'�5�/��?�q0j�4�p���(?�;�	:�T?�q7A��.�c�}���^o�˼_�w��� �5C�W֧��>����E��e=��96F�/�������W�>5����Z1�A
�G���:ׁ���jklo�T��r�xz^�wc�-���g��%֬�����=���h^��{>�y�7v�sեp3?KY��� �37��Qx��A9;�|��ͧ��nl��||�v4&ֻ7��ޙђU�ssڟۣ���w��㈯�x7%Fyޑ�~����t�8���e�.��4�SF���T���}�j�-��]wVe���ŝ妖 ~W����Y����-c?��,�)��	�>�\f�p����u��j��ɶ�5��nJ�Oݭ3�����`(�@3�� ��:
�R���G�4������%���4���tǜ�p3�&��!��pFX���Q�Lh�D��u8"s:�
����q�rYeR�SJ仫�8��톳B��;ޡ���V'�@i ��w��8�����%�Ϗ��+y�+[�o!�9ȕ@�Ƽ������}7?�����wU8���s3�\.d�������~�ϒk�����{��<����]��~��}.����8��Y�ێs�����t�J����w�_�=�.�3CCA�z��"k�w���{�o�{���i謹������/�{�{�=u��{�]�Πlj���X�?�O�?�M1ܯ�S��wO}ch�{�% �)c�{�=��3�̟yO4þ,��3�=u�5܉\�禎��L��tO��6Q�>�=u��m���+��m�����!�]J���w��w��c5���:���w$#՟yO]ɇ_?�=uȂ��~�{�,�=�=uЦ��ϼ�9����{��侀��ɰk�{�i��`�G3��=u���i���ٖ��kq���=u��mî݂�Ի>0�{�G>0�{����vO�����S��ql�Է�8�{�v���W���a�q07����WFs�!�n�=���{��ԛ�{㜠��x���v���ٹֶyv���er�=�ݞ�%�K�{�<Z1���O��|O]��{�=;� gd���?�����Y���9��3��y�e��=������%���Qk��7�Ӱ�}����{Ế���^�Oׇ���l������Y������m�������wʿ;��C����~L��=��������>���8�9Î{�g��������Y"xO���{�̐�A���g�/һHo�c�����{ �����=f����oW�O���}����0/�w �^�v;��h��;&�{�aOQ4�W���[��ۡ�P��m������q8�?�+���_���#�G��~���z�o��E#q߶��ް�{aȟ�p���!���:���p��1d���d�;d�?����;}��������;�c��{�_	�w��B��&�M2�4�w�p�޼�A[ԏ���}���}�i|ؾ��a��Ӈ�������w�V��;O�w�և�;O]��MEv>�?��<�
�w����P�
��w��,l߁½{�_�;Pgw�^�wt�e�x�,噒���3۹>þ#}Y/�Ľ*þ3uG�}g���(�#�~W�}��>��� �`[�}������g�w��$��5������a߱z.þc�J�}���L��ՔL����L����L��Ue�}Ǫ>Ӯ�5�v��g�|�3��W&ϯ{�<��Z��z4Ӿ���@|������L�{�<_���#�Ϡ�M���$���ȴ�l�M�}g������ʴ�l�ϴ�l���w��g��*g�ٚ;ξ��t�o�%����p�Mq�-��Ņ��#*�y�ҞT�m^I	������R~ϟ�ޥ�ѷ$'J�n���v�/e+a\ֺt��̻�+`��k�h�R����Tgl�*��{h�q��c��3�f9} i�2}kOKګ�&@F�{��J9_LQOg�.u=Ͼ96vfC�a�tlX����ʹ��6x`��Tկ�/�M�Q5��=P�B��h��/y)�=4k��>W��5z��F���3��d^
�Gq���7�7�G�c|.{�<=�Gu)�j�y��?V��c��z��P��?�K��s��睷��V�1@>,�i܌���O�\L�:��8����B�<Kp�@��$� �z��u=7;�����{��01F��e�#�<���xG��U;�n��C���4p쀕�{a���go���x&�>�z�Џ4w{x���b��j�h�`�h�uy�r ���'4���q����(5ܖ�����o��奔N�����ǥg�V�@�l����xZ���#�^��y������
W$�.���:��H+�kȞ�(�����j=����������ƣ��{��,q�pL�)#{�ؿEO���?�]��_��[c�����Gy���D|�=�\��k��M�՞�/���f#�?�gۣ>��މ��~?���TXy�<���ψ�8��(�E�}�?���G�$�q�~������w�~z�^H��.�Y#�?$��'���Y����(��� <����<�~��8���?��������>�����3rZ������1{�f��Su�ӻ�e<��4�>��c���1��DxH�d��ϳ�;����y����{f�|��#�x�����ٟz��;���Y���{
���I}�m�wmp�Z�ڛ}z��Ӱ7���;�-�
�ۦ��8�^�lt�Ilk̝��թ4Z�(}>._}w����%{�;)��L�2ʝ"���XR��]��ݑ��)�O��B��r�t2`����(k��D�R���׮���l`J�i},ɀ)(��3F[�d��1V�%޽�Q-v:	�5)⾝jDpThs�Ն��ut%7@)e
�
Q��jO�aU UooPB%���w�w�[���nP����I���+�NN�Z�:��Ok0�7��S<-�`�B��9����\}��e��j���[����UzB��*?��Ki~xt{�g�����k>�$�3>J/޴�o��8)��&Ba�|�K�2��R��g��m�/e�|�K�1��Rv�M�D$�ƛ&�=�?e�|�Oه7]��&���|��yp>��ϸ*��3�ʑ���ro"l��g\���3������ro"���3������ro�؞Ǜ��x1xo"��g��x�P\�8+�x�����V�0�J�DMY��+�xq:o"]��X��7-�yx��7�Q���-��ܾS��ග!��"�O��x"0�1�����a>f���C���a�g���>ü�0o3�[�&��k��s�a^c�W��\i��s�a��<���\Üc���c�Ͽ���3���07����|�00�����g�w�m�y�a�d�{s�an5�k�ǆS�~h�9������q�06������6�T#�ԀιX��5�L�ah
z��E�+Mo�>���}�3rv�+ݳ8&�Qyy�aޞ��ɸ*~����l]��5���w�D�.uG���]������qW�K�{��Ƚu࠘����S�A��/Q�՞�-lo�~��򛨟�{B��6Fo�reחiO���mT����q�g���Ʒ�?g�wȹg �]��B-��G�>���TW�T��7X��������sG������)����� �O���1���W����e�ʨ<���CX�����з����'+hs��ޛ��J�6z6ҳ���n�_�*�1jWZF�]"z�]��	��+�p��=���J���v���Y`E�x���:k�}�[1�ʿ0Q�d7V�P��<�sh�@��&8_8���w2��Ι�ѓy��5T�5�6�|tQ��G�˽G܃G��ի�>_�6���ӫ�>ߝRwS��W����=��A��8	�_��4�өX�: ;�O�:�˩����~��A~��cK�˵�)� ���?-:l�{�cp������ȭ�̨�?�w%��O�/^*�?x��i��YCa�$~�=�NN� �E߮$��R�oI�O���k�x�޷K<_��K��z��YCO�����w�tfS[�>^��`W�.��)�v�탿A�da�ʹ��w�}���~� �%OyT?�rujZ��o��1��8�����S��Y�ה���X)}��8�)��c
l�L���S�+�B�p���sIO�-�ޤWE��R:���j�vI/K�[#�����#h�5\�T�k�%L���I;_���p���N􋃼�?h�}䩟�_�S�	�C���m�-��P�=���&i?.i��8m�C����W���&p)Ԝq�.�������� ,�+��0�_���磧���L�7���LzG)�3Q��l�렌e`� _m��R'�K�>	���V���9O`};y��T]�%���x�%�Ͽ��+G�=��������7�iJ��W�>��h���"퓤g���s�����.O'��WԼ�ꂶ;�)���M��"i��c�^p��}�J�K�	�	��2ß#m9qmB۬:Is��U<(���]s2���?���,Ew�<���'o����>Z�o#~�&{��1_OD[�A���Κ]�O56�����Y9�gȾ�5_(��X�门�>8�'������څ�����_\�:*��|/�c$O����p[�}�S;�b�7�G������hܿ,Sc	4�J;i���C��̦2;R����~�>y���Q���\ڂܷdp��^���O�� ���F��m��F�P��R��"��������Ak��B`W���(�1U��G���x���)���!�����gJU����犘6�kљ��#E�U\_�I~Q�ߣ8�����a��{{�=C���n;�\@�d�?9��/R|��x�	KR~h~����y�<��.��[O��d/!�7.p���|(Ӟk�$/�8/�%/S%/[�~Fρ����o�'v�8D�|�g2i~�Du�"��}E}'�_� kp�[5.��St� ���s��M��A`>`��&ڢ��$�M�X������O����_7��BjW=�c�1P6`i�l�'e{���R搕��(e~��ߧ����&\�����Ge|��}US�\[.���)>�!ݒ�Q�s���m+~>+y}^01v��/�|�N�ebn��� ���y��R��]�yZ�Y#��^ �����Ӵ!���07��<PIh���4�y=z��Gf��ǿ��:.���e���w�bh�S�n�4�甗�x��Hy� �v�5��N'����V駠����>���z��f������~g_`������4(꧜�����3����c�zB�M�c��Ž�̜{����}�^�]�@�4�E�����˩�^38y�}'�����_ ?}�?�}�g�Z��%��ܹi@�u��Ψ��<[_�X�5�^�P5P����(��M��?D]��� ���T�����,c�c�����w���1���z�^����P�w'����˿�{b0�<����j`�+~�@q���t�h�������/s��Y�����6>4D�����AgR|��	���e�8HߐV�+v;�}��&i}�ޫ
�QR���0w6�����/3 �:X��K�����/��%���ɾ���ި��r==k��,��^2o��}}=��>���_����\G���K�*��ӵ����G����n}2ñ�D�S=?������kb��X��m��R�_�$t"��Ga�+����㨾�=M_������i���>�~g��&�����Ã��On�>��x:
�7��?�)X2׽��t�$_$��A�ʥ|�@�g��Q�)d�2���cb}ux���M��$<��qj�g^�v�
ގ����I��H��~��Ǘd?��8�n��t�)��	Ky�+x��w��A����~"��Q�Y2?c�B��&��!G���%�?�o3$ݿ%3�$x��Ǭ����ܧ���"�{�|���>zc������~W]�����'h�ޗ��A�䇙�D��ɾk�%.S�K�m��K�������ȟ�u�<�p�P�җl:o��5�'�o���z�߷���u��x�7W�[V�j�|�y�|��K�Խr���)�ԗl>�o�d�O�R�3R��o�I�?�3Nс"��M���W`u��&��"��U�2]N�����O �2S[(�̈����à�v;΅�q�s����[�)��"���8�%^Ks��<<H��;X/���}���ٴ�Y���b_3�%/sB�"��S!��{eOV'e~RʼW誡�2c?����:��ח^���J�%J|7J]�J<�n�s�@����g���{�˖�>$����o����F�'t�r6��9�_%~�����)�}���8K���'ϋ�7��(��F�O���Cn3�~��t����O�h�o{�t(��||2�Wdo�������ĝ����a��r�c����?�1��{^A�6jZ��\��ٿ��~X�ZO���y�3RN�#�z=.�o@�x����:�s��..eD8���E���߭����\��6��1КT��h�]M��/2񮫉�q��-x�h�L��~�܎ȸ�>���/�t��פ\_��g;)��^������uXgs�xP?�}�u5��'��I���㯪����s[hO!s۵�g�CS4��X��K�eS|�K}���*�a��[�~Z�2���C��[��L�I5]P��T�`���3Sh`��i`�0=���{�04o��eQz/#�l����}q�|z�7�E�y�м��>��uK��;(��ή�bп��\���u_��^�н���8n5��=��.aQ�}�_�b�����з�?�m�;�}൪���Յ��������4��U�[#�#X/(��s?\w�獂�-��z���F=M�2�:�ޠYK~�Ө��-��c�zqb|�� _ۭ�4�\��)����͊>������o$>o�>s�_s]wӀ�y��]�/�op���x�]?d�c}��U�\�uy�V�/h�ɓ=�v�9�4ц{x��{s�l�O��f��O�-�<��\���g�'�Ry����&u��\�ReFhZ
eL��j��Rc�v�����~�����L�^�8?���G��_�I�>N�9�uZ��k��U����9�'����F}tI�3��U��9�T��z}j�yoT�o�Ⱥ�R��)�)kH��?5�4��y��S\.�7��о⪓���NN/!z�,c`z�}'����$�|�c��.���?��V��$�o�5�?^�||C��+��ٿ��y��t!|a0ߡ=������Y�(��ח_�i�|�{����l:�ċ�Ǫ��>ў������3��2n[�F�~��^/�fH��!i��4_�,�^s�^�!?sp�Ou}��@�~��a^Ɍޣ�nZd�C��%y9!y���U��㨏�y�x~��3�@̗8�R�/q�IwK?��0jԡ��!�zƂ#:��׏#�
��������T0!�7 ������d?k�] 5v6�@0R��kf6}P]=��@���:��?�p��U�s*jk�_X��mP`��P�%
�h6�ĒqWk�u���g3�`�Z����)��̴�
aV#M~$�c����%�(�wJy��/K�3Ya�T���Z�.ГEkÔ�s�]RE��;���nGb�dE�F��������*Y����)�J/

�җ�.�0~ ��>���v� �#�P@����X��_ɪZ�����x�W	�*L���#P^.���u6Ż����SLPǌ���%n�o%�ʠ�f�i�P�bֶ݉.� i�gv1�UR����j2�[�Ꮕf�v�/V5��Щ�0���OR6ˋ~�a�[ߗv�_��Z�uU�S��pK��.M��䁚�.������JG̒e��>���a���Jҵ�E�nF1N�3꜑���,)�Z�v;��oP���/���u`ֆ��}��i�#��a��4�%�2�)c/�x���qr �6��Wʑ�c���йj�"	�}"�n��G��g)}֘w�:o1J��#�|@��2*�2.�3� �����S~`�>e����*������n�3�N��kv�6Ո��x �l�������\���O�$��T�;���i1׊y�?�x��S�{\K���m!~C���p���p�w�p���^]�g8�A�g�}ߙ;6����,o<gx��l��z���3��ag��`i]̯�Կ)�z������8�?��y�����ƹ���L����#���ʦU���_�5~[�����5F��q���/_\c�=�C0�8W�3��Q�U��c̏p[m�_�5�߇k,��p�A�d��f��kZ'�8����ƠQ.L�{�:�t�Ơm6EF�5m��t9��Ơ��D���t��5�ة��X��os�����5FE�Ք�i���c��X����k���������5V�����ny}��ƌc1:�1h����3q�qoh�0��t|\c�����#���^=��:�w���&�1�B�^��AA\c�3�.�[׸����46\�-7��x��c�5v?<6\����������"�a��i��%�k���F�7��],�1�9�s+c>c;��N1#�l�p�9C}��km�gg�}G_���쌰�����h��x�gg�)�q��=;� �d_�q�W{v�5^��yEi��<�a<��w��B�����q���5k��E�����q��=:(=��n���k�mğ�ا����
�g���t�ƺ�4�qu��Ձ�v�����e\��^|�k���0�q{ ��@�;;��*/>�5^U��A\c�+��B����>�&|�6��v�������O������?i��1���E.�"���
wy�ƒ��qt�������_i���I>o�.�m�� �r2�~>���M��e�}_ '9�+�{M y��m��_�ݫ�|;:&d����]�O��6�s�~ưc]{ް�ƀ����HO�~��o/H���o��W}�.O�7m��ݶ��ݶ��������������������˨<�3�v]��P߆{5�װק���o�Wn�Uy��'�S����^�}w�}�=������c�o����;���?��1�?��v]_�4��'�5�!�F�m��Lq|��Jc>ۖ�~��Q��Ϸ׃�����߿�ׇ�����2���d�DȘ��,Mh�o��ʸ����#0IaW����a�lJ��(�P ���a��c&FYS��'�a�D�ǟ���L��0��^��)�'���3�q�GaO���Ӱ�y�=��y�/\���6����wd������˰1�>����*����J~oʰ1�nͰ1�*3lL��̰1�����]�{0��l)�6fڷ2x=�b��e�j?ΰ1��
��|�����~��6��P�����ic��?��X��dY���[&�ڒL^wJ�j2m��g�l]�6ۧ3m�m�,����L^���>��ic��a&��%�gژmOeژm��61�^"{��8���nW��1��5��D{�8��8������P]�[��o��}��G�8�5�����p��p���1�v���~����qL�q|�v�3�n����{����@��z��wz�Se<��8�צ���q<�WK|��a=��3i<�+e�lw�3��.�7��1��gyL���m��[�3��Z���v�W��1�֏�1�>3�Ƭ��x���ۘu����t~�h<��;$���gz���ߎ�1�~1��Ӎ����,K��;;���|�����ʘ`c�M�`c�͞�w	u|��}�a_2��̫�`c歝��^��`c�}n���w��!���`c�}}����GlL��x?�F��o&0�ޮ8�Z~���Yޗ-�e,�9��%��WlJˈk�b~�wQi)���묘:lQ��Η0%%�/O�r�H��b��\~ϙ�(�r�3��@�O�U4��.�^d!l�n���� �����Ɉ��� ɥ�4����D�PKD[��'r�X��.v �0�n�|�"+�����ĥF �\�XKt�;����X�'��G#��H�	²;�+�-�f(_�1 �$m�(" ��%iu��X��r7�L���TH�T�G�U�oZ��6�Xp��QՍL������! d�_%�42�O�
����봐'}� #W���rs��!C�Vϗ55J)cu��Jm�؆��K�*`8S��izB�v���-$eJ@Zx��PXQ�w�5'[͵A�yZp��6%:��I7���U@�����/MUz���Ũ<��U\����?$��M��K�Y�ü��y%)���\���f�����+�k{j�������B���0�#����[H=�(����܏�	�GG�V�XH�V�k�k�f(3dM��@�@y�	N
q�{��g�P��g�;dQ��:NH�H���D�@@�9�[��F|8��'dU��àh;蟀N��q.}�ء��B�A�P��rF��;���~�^	������g���;�OY5�Q��~�d-�=o����t��6�e���ڔ�sw���i�����Q���qr���ϝCv���A/c��Ok��Q�{�ދ��6���Q_oc0�������G}=�?�'��ϯ5O�]���i�	=[��9���0�>�3]�W9��}����?�3�g���d?h���#��o;����'�y*-�ד�o������a�|�y�_�>�Y��K�!��?���<�_���9?���S��<.'�9�x�D�y�����o���t����������d�6��y�*�~�#�����]Fy��������f��1�_�$ٷ?��e�Wz��۫1P�$���4?�q~I��zD��O���~!DAN�A��<󘯷qe �f2�3�*!ܥ�}��/T>B�@��?�X�A��~Eό�}��֣��z�üF�����q�?f`z���h�H��_��O�����q�d�gV��4m9�{:��������z��K�m�9Fs�o(ݳ���挕��Z�l��cqML�G�`j�3����G���#�J������i�к$�J;�p.�f	K�����vQhi�����Ltt9�֞ds��N��Nk�`zZ�"Ҋu�P9!���	j�6�A��7{ʃ�H��b]o��cj��I�[�p5uG�Ds��4��1��1��*BF�Z��P���u�h>x-��U��"Z�:(J�X��B��BEy�XT�:(*�XEu련�b�U��b���*�A���uP4W���*�A�^�:(��XE��uP�V�U�{b�D�lƛ�-U�{bk��Vź'�W���U�kb'�D���M���*�=���uO�b���X��~��<�7M@�X��!���=\�:(�T���U���X�x��uOǛ��U�{���uO���U����*�=q��uO�Û��x�|�6^�w��E�����E1	�^e�0��9�0��,�����泆��a�7���1�|�02��>ü�0���y�a�b�7�^��e�[�ü�0��J�\n�Ks�0�f�0��Üe��|��Q����a�7���1�|�02��>ü�0���y�a�~����z�c���Z���՟�M/E��X��,�=�^
�:��b�G�>2øǖ�:���0ONt��i��t����BYN�| ����\�:�{��!�9��<�h`�,���ܰLW���ߑ��`���`����N������˔Fo��M�=��� ����7���:�~rh(R=�y�R0=j$�Eo�,�{)�ߢ��ù�;��,�9���}�Ja]`#���-%D�Zrл����l���ڐ�vT_�~>t=���Ρ��,�Sz{�9��{߀Ƌ����D^a�;��+����}'�����3I���h�@m1t*dу2�>S1�q}1���b.e��1�my�_��NI�_�_R�.e1�eI|�_�ėe�w6���,�O鍠��n�����^ǧs�������5��)<��)���쫶=;�yn��;�q���(��З��3��PB��a�q��BoZ���B}3ǽN}��̔(cP���S~54��,I��E��b,���&?�����~7Cᚾ~�{N*�)��{��@�}v���n��xg�&%lR܂a�%�z��EW��R���3�����Q^���ړD�<�\q˖��R/3p��}h��9��)��e�y���2�.�x�-����D��������y!����\r��@g�8h�r��[�q@^�F�S-sO��5�Q���K���Q�~'#u���.��~N�}v5�{d�c��sX���_�|q�����M���̞OK�1&��N���n���M�ed����4.�ȸXGy������;�m���B�d`GJS�ߣ����������̻Ɵs��Bϣ��_�0�<un���W�R�^��r=��<@�C����L�i��7S�
�9n����zl�i��Q!��f���6m��*'��:��{~���odr!�%��-���I��p�>E$z$W���@_�1��1c�z����a�.	蹸V����g��$��ҧ�˷t���.�p�t�������S}e&��}���z���y͢u6��ٜC��	��n�?��0�c`�i��x�!���x8��Ǆ{����f\Tҷ�Cv�?B}� ������}�~TXu2��S��f�1���s�������d�ȼpb�?G �3�^�P��@�y�<s��~�%=��.�������U}����n�хS�!����^���c�[F~��s�+����2�y>�I��jK��?Fm�7s��rW:�X�'��O}�cvw	��>���qƮ�!��_�
��ǖn%���to�p�0o�1,��A��Ü��Zw��*3/&<�32��?&y@�钸����j���L	��\�е0C���Π�:��X��ZZO��]VܫC�����Q����8����sV���~T���A�_ݳ#ݳ��鏍E���"�?[-aˋ��=�6o�?�Ӡ[P_�E<Gfz� �-p;#���[��V}��'�b���i������x:��'�^�j S��3/3^c�|~}�౮�v�.�[�=�%O%�VU˚��zcLfѻ��`>�y�c/��?����S��&��j����BW���4��%�T:~��l��O�Zp��F���C�YFqn�i��g�E����:�	4���Y�Rh���Maj?�ӡkD��}M!g�/Gd}9O�ZGrh��F�=����Z�i���8}?B�Yz.Г�$��g=�O�:�c�z����w�8�w��Ԟu�%�S�kзG�}�4�N���h����o}M�Y�I���vɼq/��7t�4K�kB<?,"}/��e!�!��ni]�F��+�G������Ρ�_�8���~^�i���g�]G�/�J_�����R��!��:��J���[%|��3�:'z%�j	�E�taڍ0��"	�u��6�O���('��`��~C�����KG�L��s{v�	r��$��>z�}�ē6�RM鴾�8���A3���Z����{��(�^��Y��o�xī����i]2H���'�i�g=��'�'���������b�&���D�� �!Otb��+�5�I�ۥ�l�~�u2�+��C�Лd~�H�͓�v��Z@���Ez��-�!�/�{{W��{���z�+�������T��K��)~��j��y|��k|�\�^�.eѼ;����V�~ěQJ�-=j^�9��u��u�:h�R�������G:?[�}���
ŭ�<�c�Ӟ��v�8���I��~����A����eN���$��njẃ���e!�(��Q�}��e��v����m��m�\���J�R���j��)�6�e�^��KL��e��"	�y̕��Ü� |C̛9zm�z=$y9(�ȕ|�sxD>��~�ˡ����6��~A�C���f���i��8��o@h�����r�G��#/���4e�*�.�rcZtx��o1�R��)P�d ]��ȥ�q}:%W�w�e�-M\)��Qʠ�)�Sh�x�;��\`��t��~����y��ǃz|R<(��i��N�_�����p�Ϡ,/�Hߨ�z�:��"]��S��_4Jݝ1��fH���륺���鍤�z�P���9�U��-�xN�8a������<��Kߛ�r�?�7���
c5����yo�|���z�c�9�)������ܗ9+,<	��@�yV��=�N��E��ߧ������S������?������ޛ�i��z��w���f*끹>]
�ܧ/u�*j�)�5o�h�C�����7�������<��:A�2���K/q����'{=�^�1Jq�Q�m�^�����;�����j��s��T��;��_��
��Nl�::6������[�:y�8���C�� y>0��l��]-{�٧�P߮6�l5ҧ�^c�ْsR�l�sxߵjN�=ۢ9��*���g˓�S�߳eI�%�;'��v�W+�K+��U<c��v;�[��)�׊�Py��m�s
�ь]��z��zS�O��v����C�%��T_*����)ٕr�4#��=�o���CϽ ���S"a.���;�^�5�A�zM�J&Ο�ަ�G�3��燼~2 �&�[e�w����D���y^���[��K�����s�J���ƙK��/�)e����s�zAQ�E�u��5�]�N�i�N��|� �WJ�J=:g�@��1�����{zB}C;ϐ<�s������y�{gjש�"�Y�}%��/ܤu��=@�}�|D� Ji�����S���<o�x�T�[(ϏS��_�#yF�gh�6�3O��RZ3�|F)�9F>�t�j�!J:n3��u�P��r7���I��"|�<ig�&PV��:�����^z�cϹ����/������OR��F�u��Lմ��������X=���,ќF:�������rد�]H�~~G��ꍴQ�LY��v*9�����qYG��/������!�s=��/?����a}�(a�Ga�s���u��4����XR�@�,Y���Aۏ����Au�%�O��?�÷f���:�%�nz�����8�|�Q{���v,~������Lz��i�g�O�>��t��ҳ��������6Ͱ�h�}D7����oJ��&^�y�r.q�+:p�w=����L�_}_��:yR��?u���$�(T�wK<W.�o;�]���rJw��Q:Hw�����C��~�H�N��$3�c���:^J����N�-�##zO?�e�Q@�7*�}Ϋ�9@��~�U���g�O�O��{)�T*�;�����ϷѼ�w�8�7�W�X��?��{^ O��W��������{����O��$M=�������[�C�������B4@�.��m��������%���Φ�����4�����?��~Ȫ��R߃^�(�Yj�Wm�C����	M_*�-�|Fe_���q}�T�����|���H���t��t�҉�!�rI�,��>;=��|ٗ ݳ�ƙYjZ%���/��ʡq�C�M�x�[�������r���?����k_&�}�\�m���ǠMB2�~��v_C��Y�n�)�(����e���Lk��i�+.�k��3ꕎe�ۘ�1�o�5��W�|�ȸ�<�],i��� |�C���_�s��o.����0w���<1�������9/Y5L���].K�Q�iJ7R��א�2��ׯ~��G����Ά_C�Uߩ�_��w�74�%?�3t3�K?�|�U��үw����k�	���7�ZP)�����8ǥ/-��8�]zZ��K���l��Pz^D�����>V��*bHh(����'f��B~Q��p����w�R�%��X��yH�����>r�W>/+���ש�E��2���j�Is��(���4��#��:�[Nu<U�COx�$Zw��оS��	5���.���gA@���[�J]G���R���|�mT�;����~�,��5�Cݯ�>u��3G�)z��*p�RBqV��!#Oj�ߒu�6�=����1K����o���#N
�Y�����yR�uG`�鳮�ƺ�����M��<I�7��Β��ٸQ���y7Z��4}u��˻�����*W��g���]J��F�����fH~��+�_�v�!����p�S�_N�~9]�{���6���쩉^ĉt�GA�L�o���H_�I�z����y��cܯ�NP�'%�8����)�3���s�u%'P�E�|�2����g���ɸ�9M�%WR��=��!��?��SJ�F�<j�|j�v�K��>�Y��k�_Y�b�,G��\y�3e�̓����r�a�L`�̔��gy��\5G<:x&�Ϝ���ƾɕ��V볆`^n&/�Ҿ��<\^V��S#/��K�� �3L{U訮?yn�<�\��)2.ru���c��\��񙏖��g�ҧre�EYBe}�f�3��槺��vs�J�v��]o��C�9�n��S)c@�����|��$Mۘ�Z#�z���z=����c�n!7�˙)gh9���Z��1��
}+t���tgh��3���������������)��zȗz�Y�<����T�2�o��=p�Ƣ������u����CC��q�L0�Md�w���=Nou����nǆ����Ǣ�eX
a��V�S���i솪����Z�rJ�aS�Y G,�)28�|4̌�c['�eq���Z�Z��Bu�N�8IFq�n����ےm��uQk��XX�&���\@�W")J�f����;&�3��p�ֱ����z�ƄRlukP?d��$9��H����tl h��e3�P�&��RYDQ�t�$��f��S�gy,y7����q�W��sFh*�&��ʤj�a���F�L�P���Gh6��AW����=�]��>;�٫v�����i=�ao"�7���=��Һ�ށ]��f��uv�U?�;��-���Żp��=q��{wk������Ryh7ם�H��|���r��66�w�A\�F��S���r�Ъ������ӱV����t ��z�����nk��ͱ�ƞv4�:���v���2�Ӫ�����Ʀ�T����S�E]�Yn'��a���÷a�Ј�Q�k��כiH�-���nHƨot�;x28�����w���z��FN
-Ku+��g�R��눡J�qt�p�n������Ic&�ӹ�:�zvLxvL&��c3���c�-@L�$��[�f���B�d
���	���r �U��W,��T�Ɖ�L7{,ִ��M�������+�s�Ru@�[Z�z�G= �k�Tߙ���n}e��^q[�R��^ZG1�v��چ�5�ü� ��ٺ�Ԭ+暥�o�t+�:��T��7n�PnK�.Y�PSQ���\�$�qVv6�m��F7�6��&�zWc����.d=����N�m�~�ޠ�����;a��$K��ɒxw�R����������n�&e�Ղ{�Kh�Z03�֊T�]\^|��f�������I�U�a�JX/&���O��R���)�	/��11j^)ƅ�m46�g�>�ަ�)s0~�'�C�'����/:��0����<��5�n���y���y��"!O��sѼ�"C�\_����`�o^<��u��������.#��jꍬ mW#�`8o8rґ��K��Qي����(���|�c����W/�p�T���+�����V؍�LJl�H��0��*�Q1]?�uw��tݝ���7����)��	�`� �������:<�E��6����pz�n��F���3�6�x��/�:<3�ux~J��\�����D���i���I����<I�3Jz��i���Oi�}�6zv�s�ŲhO���~zV�t7=硴�r��y��z�\��D�'��Hz=O�^ϲ?����^Ϸ�^�}���|*E�'z�����C0E�'
��e������q�����n�������z/������	zf.Ѝ1�^O�?���	����	��S�:�醝T���w2'q9F��	ziƤ��z��ʟ�tW�3�^O���Ơ�S��o[������	���w8΁٩��c��X�~xt������H$՟��Sa�~dt��������z=�tt����s��g�����a�=��z=���0W���^O��n�m���z=�w�Y�}-����X2��_���/���~fص[P���llz={��M��c��y���z���^���Ǧ�3~���z�����5�	�iz~	������ت����b�zn�������-����u�g��{v��q�^�3���`{�v�7T��,���S�0j��9��g�)���z=s=�B�t�zv^Q\��3�[��{�8�Z��*�~y����;��Z/ډ�^ϧ��"�ьztPz�����t}h���^���)�l^���z����O:�����^�Ӂ���1�ZB�'ڏ�Y�����z=s=^���z3u�A=�%�*�x�^�)�g�tz=��Uz7ik�pf2%�����W�h����v|����[���$>��/��r�o�ה_���w���s�o�a�W�{y8��r����_F}�+=��l�����E�a�o���~ ��o�3���]C�M��9���c	\��@}���X@��ݾ}:y�(or����|���o���e�wǷ�������Ћ� P�
��@}��h.�c�w��=���X��1�r���ɗ�ϼ@}��}���o�6��fU����ްg:?P?���v�ZF��O��o�����}���ɰ�}�a�si�>�}�w�&���W`�F�����y��ƀ����=�(����?��|���ʀ��?�����o�	���?�#��ꃾz9k�������o�;�_��Ol���Է�s����?����3�\� ?��h_<��+�`����Q��z�Mx�����^�8�>�zF}~/�=�3z��/��5���w ՀK\m̏�;��s8b�'�6n�=��/�e����<����{a��p�Ƶ�� �����3%?��m���|������t���6���"çg?�1��e���fJ}dظ�2l���fظ��D�K�W��dظ�?"�ѕ!�CE�Of�8��a���c����oЋj�zCM\�P���M���?#���͢�0����t|ɀ�O��Tg�z����of���[���ߝi������d2}���VzDM\��eڸ���i��̴q��2���?�i��?����n���6N�8ly~��y�f���?��;�˗?�qF����쥆�M�x}h����1��v��8[�@�8[�@r�m��Ǹ8:�}�l=_$�a#?}Ѓ
����8�K��߃R)?����{I��/�ٸ�7��k�^�Sw�5����z&����o�1��PS��m�m=���zZ��z>5�����zd<�'}��_o�9��񶞃?��֫�������������ރW��z�������{'0��E�o&�n�z�H�_��ǵ�kR�7�z�`�Q�PS�º	���'���8�֣��q�Q�ߗ'�zyV��-l�=ߟ���qq����tjӧ��~r�]�g&0=��Ȟ�`�i�ʲ�4L˲�4�β�4�Ͳ�4,��;��R��,[o�ǳ��ʓ��g�z>��xB��7f�z�b�R�ߖe�y�#�Q���,[���,�}����,[�/�x��x:���l�򤿼]��dm��=8E)(.-TzP�K�Jj��� �(}.)������XBA[�z������k������i�0 Ne	-q�s�8*�$����(,���|��$��R�>_im-�[\�o�h�\��Z2O�W\�V�pm�T
@:6,��[��"���2�c��j(�I��;�]sDA}!T�{�Rm�4v؊DUɕ�f��=��ŷG������%6�jS%Jǆ(!�ذ�U0Vb+e�miN�ViB	1�D�ܠȵ������u��ԅZf5(]ڱAf�Jc�\�Vk��y]:bSu�Za��K�.UWlǆ��sO{�ҦB饈,zB��&�B�Ӡ�֩��l�S�rgR��)�[{ZLqp[a���eE3�j�MU�	m1V5�2��k�k� �
u�ZU�:֬�u����	�=��%�x����/��(���:��ڎ+�&dK�TnJP	�[�lVJuR���R"�������h�nL6�&T�M1���Y[P2E�֒�u=AYZ(-
L����:�D�����FhmJf`n)�'�c�����lh��ԗ�1E�l���yx�񞤚0FVEk�и���Q�K����_��υ�ג��Ѣ�b��os��}3~���y���|0���=����k����_�_Ǐ��U1ܐzv��������	�GG�V���T���d���ǥ���x>(�y��|�i��͒����	����8rz���zZ�o��eu�ڧģ�P�#���	(3Nٱ��=4�p��p��u��D�uX�*N���`�e7�Ӹ�L�y���'�!��1��=d�2�?7�&�?�'������	ٷH���$�S�=��<W����|�i��<�ΐ��y
�4u���aB/��i>O�d���9�����k$�|r�����4��}����n�C>����i�1��-~�{vL{�y>%���|t�z���}����<��~���%}�=���C{,P������7��,��Y
A�C|��^W�=��ú7_1w�=H2PS�yF8nw�yF!����A<��L��V?~�Q�>��x0=��lV�s|��?iuw�6��M�=o��Nssj��#k�lii�I�b���J9�V�ق8���n��ӂ=��w��,=z+EV�/�~:��h��b���|]jYM�q,M��y��b���Xr(�w)����~N9��+�$����2y�Ly?��)4��Nc]�S��.Kw벜1�uY�Mc���X�et�,�ƺ+K���ʲi���o�Ģi���r몬�ƺ*k�����i��r�4�Q�oZ�Lc]��x���:�uT��M_�4A'��n��i��r�4�I�i��<�uQ�����2s������Y?�Y��1��_��g���o���c���a>d��}�y�a�3�;�6ü�0o2̽���0��5�y�a�1̕���0��06���;4�\=�T�Sh�F;�E��N�k��%>gI��i��0��Ac��vl��&g���I�[�I��W�\����/Q�69ב��A�-����e�������Yफ़K���s��?�>�C�.�sP�_�n(�{������ʩ��%��&��Tť�E'_(�p`��*;��Cѽ�_c�h�G�>�L2�������N��BZT�tn�N��X�h�g���%8F?�z�
V�T���:��U��)���cӠ�q�vF����\�ز{|I`,��X�Ղ+���&��9��hQ�T./�u�������4����Zz����NF�^�1�ΰl�9�3��0��#��6K��-t�o�;���>��>�<.�)9R���a=g��c�?�nA�w�����ѻ��"��W������b}D�c��P$*�<�#�Fp�Q�Eo��ݲ��a��};x�9���k�B�>�s]���.m�w�֒�}/}?D~��s�����<��ӓ�{lfC���E�z��l���p��z�%�ae`"�D�g���a�?�W�����hz=a�z���'���T�`��~-�'�g�~?��ۆ��u~h�J�D�0{?I�Pb�,־��9v�Q��,q��e��ʺa|ux�^��υw)��W�m8�>�`�}n1��M�y��s��z�E�U)��u��&��52�s����*c�OxG5��t����%4�5~�ֱ�y��~\K��syR_7�
V��{�ϋN�3�|��<q{���܏�v�[۴ND�}���3��C�g�}��-F��	��qd�@����h���-�n���Ym��o��E��(Q8�����=���^z�U�����jCP����_�a�k�o��������x����ӳ��$�&O��j;=]�����aN���N�������I����+�h�+��믰n���º���
��2����Ez��~���NP��_%ڜ�jz���B�nz�5U���b_w �C��n��}B�/0�x^ô
���_�q�A� ���`�W�zt���>B�X��&��פIe�4'fʜH{�g��;N?=�_t����Ft^/ŧ���/{e,/��V�����\��B9���s���Y��/�0��(5���Y��>�00���<3�>&������q�c���y�[0v���Z`fB�$��|b�����
�GY9��Nq�y���>��m6h�M�V���m��"��̕�VKZ�~��u��k�U�&�� �S��'K����AV/t̕P��#�ٹ�	Vh�I�a�j�Mg���ݮ���R>���'5�-0M�h�:i��2����5x��j��.0}c�A����󂍙�͋d��0�5e��)_sԘ-���}���пIO�*��^MO�;d_C�vz6�3��[��k�l��+8�a�#��W֘	j��v�۳�}"���h��$#����i�?'v��c��˻����NoO�<���X��,m���%�Y���;c�	F��E
k��oY�Х�w�f6o��F{�2�#�^�[��|����"	���I���r?�Ӧ0e,;�����1���Ĺ��l�^�8?�\�a��ϲuݮ�߱��]c�U}}Xoi1�t��7���z�nR;�3��8��kCÉ�(�p��v�8k���R��.��2�P[½�����������bd������Ɏ��k�kqk^����3�{"�C�j���ov?�+�{���������;�A��2���������9.Χpf�sNuKٵ��2��X_;��������B鱾���G�cc}��͸c��2���X_�����>x�u���3��[����m��]�����𱶈��I������la��W��c�-�����c����X���/>ƞ���!����E���Q���=}��cp�f�_���߇�U4�A2�G��Ͱs�
����XX�qm���t�X�M�r����q�r�A|,�$�w����s<kt|,E���������"��W��w��cuS���X���_nF�?K�{^7:>d��]7:>�o����2s&>�ﳆ�����c�\�3*>h��i,�jƇ��ǂ���i������Y�N�������LA|,��mî݂�X��ǆ��c���6�>��c��������|`l�X�`l�X���,����3<>���~ft��x,���y�� �3�s��{��uȳ3>���ŵv޳������X�{v�Ǫ��x�j�T���ͳ�D�e5>�>�.=[�G+ӳ3>V�g�%۳�̗=M�yǷճ3>V�g�<�H�i�&m��B;܏{��H �(=>VM >]k��˧���c�	�g���t�X��4>��@��{ܰ2WL�2>V��ci���:��D ��G|��~��c|,��g� >d��O��o������+|��|��C{�v?f��2�����v�;��7���3�?��������~а��3��v;���{Ηm�5����ğ��J�`�z�����L
���=!~ �ͬ��_�[`4췇��E���|Ʒ�`���o�͐?ހo����j+��.�gB\^�/��?��|�+|Y�����Yv=�aG8�.�ύLYpN�gC�]ȼ�w��w���"��w�}��ߛ�������k|�;������}wx?�v�x,��]�Ęw>�e�w��������΀y��ga�n�ߒ�e����}��YzW/��L~	|����~	����8����N�8|9����b�p�1��c�M�%БT� �l�-��D8+8E`�O@ԗ��8�S�NB58���T�n�IH9�rs�0S� ����~�o��;�)�d�v&���߾�}�v���w�;��L���g#��g.���Z<����k�޳��{�J���g%6��g%��������g'���g'���{k+/����vA���7�Q_����=[1�7����:�O��g/��{�^L��g/���޳�D�3ޥ�W8g'�����g3V�g3���uP���������D�oȻ|��O��c��x�v��g;�?��Hr�-y�:���N�<�e�����Ἓ'ƕ�W��֗#��x���S1H�=�C���N�����&�Ý3N��3.3����������0�gWoh���v���93�����ȋ9����{D;�r �lʶ��50ǁ
X�.�f x�;��Qȑ�3��n\y&��!Z���S�CC@�h�#�l�#��=֚;�4�1��{Z㟸wÖ�Z0���Ʀ����xߝq����.{<+wԫ�<�{;k�������}�9��w�O�_�����U˯�����_��z��(�����?�����������h�y����������f��֚��|>�!󩁍(�ϯAy͐��*���������l§^����?�I_��yk�)l�Q�H�$�	���Y)"���A�P�`!�9�u��~�?Z�\�8�r�v|.������S�=���Rn�g��S�Q���1�� �_����U�_%�~}ƍ>�^ �+���c=7(��X�����}7J�I���[�����F��8.���h�ǧ�,��ދ�C;�}-���g4�k�\�#��R(q��a����U��H��rߵ����_,�?��ﵒ'���|'�F6y�}6��#nğ)s���e��
��9���_�|���E�w�\>��E�,����-�/�����O*ʩL�Z�Ь��I<��l�$�N��rb��2�w��-��ɯ�k�	��\j�����.��uP-���|������rwO#B�w-4������ѿ���nP�~����#יk�A[��?�_?(�[�)yya�����q�o�7���/2ߎa��W_g�խ/?�o�S�_/P]>�n����Η�x���,6�����?Լ�e��(?x������Q�����$b�1^����-���☍-Pݸ�R��������H�>�����A���-�oPC���=!�'�)eU�5�~M���-s��6ք��!�u\�俇zuP�.�������㠵K��h��d�A��o���������c�����G��w�;q��~L�'^��13_�_�m���G�:��O�;�j�_��]@~K�������|��	x���u�Z��^��\/�[���2~��{%�S��#���^ye�gNA�O�R?*y>��:��R�;������/����Uh��f|��Q�����'o�|'Yq���X��}�+��������хv�����|���{����v�
�gI�S���{��_�@^�g���j�)'�x����H�,^.��oT���[�y����?������w1ʓ�m�,�{XW�x~m����-��D���1mhÄD_l��zf�W�aV�/��f�\L�@�Q���4��l�q�r_���<���s��]w;�2�2��w�}��3Ζ.\φ�l�ܺe����E����Y�s��b
-�_�q��s톖M�XH�Bh<�+���}Kk�}9,B\vt����;�x�mH^Ҕ�vD��$�[��痳����w� O+Pi�5��6ӱ�S������������
m�w5j���CΥc�p����un�C��P3m��@���N�@)�7����1_�[� Y�c؂2�;B�+�� ���ҨQPMH�D����E��h���2�ȋ��������Xo1-U�&�;۵�6��M�����<�Z�:�=ѭܘ���>��*�<x���4��i�lgc^u�ף˔���wA�V3�ǟ��+�wn��$��y�N{��>�U;��S'Q{���$:__���.hdJwfL�K�}���W�J��{�>̉��TE�o�����`lS��;���]�Q[)�<A�ȁ>2�楫���y±�P"/^�Q��By"�H�4"���{�y��
��G�5oi�|&߬�����b0�����t���א��BK����	-Z!4,t��J�˄F�V��lh���B=(t@�!���� tP��G��$tH�q�'��"4%�WBG��!��з��:)Ty	-Z*4$�Lh��
�a�K�V
]&4"������>)��R8�+HaGIak�$�O���O`�¦�%���$�>���w��Q�*B
=�&��XC
G|)L�()����h���ZR(W���%�mRG��AZ��Ha�5��wk!���F
[;N
����"� l"R��ݤ0���ն��&�A
è�>S)ý��)�Ia�%I�����78@
� )|�R؈�H��ޑ~zG
[v���R��GI�@�D�
zG�q�)��7B�Ho�ޑ��ޑ��#�zG�zG�)��m�;�OC�H� b��;��a	i=�zG��#��QN
ÿ��f��zG�y�$��O�� /��a����c�y� �0�WL�:锓>ᤇ��Q'=�;�'}�I'��^'�뤷;�n'���N��I78�:'sҫ�t�I�8鈓�t�a']�CN��I+'}z<�>�G�t�I�p�CN���t҇��>ओNz���u�۝t���r�q'���t���9��N:�k�t�IW:鰓.w�!']⤕�>=�����۶Npێ�y-�uA��}�DKU��e�"�u�GT�W��[���`^�e6m0�"G��K6>y|W�S2�o�Ć�~�B��樇Rs"�k���ľ>�-Ā���w��O���s<���ܸ���{<K���k|S�-���4��I���Q;3�l���~իq�|��'����_��������2��G^!�
=��Ad�%��z�"�iby��j�/��5M�b���W/��H�ft����/FÑ�L8�-���Q��3�o�к�+���c)Oo!O�R��Н)F[�ОL9�Fŉ��H�����k�������t���]�pd��^A�Wh[jn$9��xy�V�~�zw��b�|�N/Fy���c��(�Z�A�W5>S��B<�S����՚t)��<��$��Ckҧ�����4_1<���l���4tR_��
2�5��v���pشUD̑К���9�G�+Eo��Z?�����0x�?�̫D��̡�^bT���V���1��0}�k�}����I{s�^�=���P@0����F^WN	���Q��ϼk敼�q���w�<?DّwtW�sK1�\���kҽ�9��s2�w˜0����e�2��U�D���|ZT�vzp=&�5��Ǉ��XIJ����lպ~�k}���<b�!'��y|��%�Ow�/ɸ+f��˸�iv@��D��q��2�3��q�9eƽD�=_ƽ���1�Ntg�)����c�r��/q�^~(?�Cc��\���/>���l'��E�����Q��?� �5��M)�G�5v�Y�k�bSj����5e��9�7_�Y]v�!n����S3�A�<plĘY)2��>������ŸW��ܔ�_E�֫�e�����V�q�9=��^�{����U�ƿ+�b�H������3�w8��S�I�j��%R%س�N�y�NYP6�1��s�y�ys&������_�H�7��̍K��3��6�ܞg�����vP����@Wwi]=�}0|.u��"�Y1��PFy�b45R��)�}�k����񴊑_Ґ�sm[:�,�n���ℑ�z�\��]돲\���R���M�_*�Z�Zn��:?冶�U)2��@�%�𫓺���="�Gȷ;EL������A]!�ͶJ@�p-}�����{H�f3'��p��Lp���p�%>�?և��+c��)��K��]+�N��R^��e����Ck�,Cy���p_��|��=�������A�F�ש{Lx^Ə9
t?����L]Z<e����kԟ��A��.��:Y�:Ū��%/)Fz;�-�G�կj�F��}��4�ʧ�s��"�5ib��X/�'���9���1Ҍm��a[����ޖ�\�%�G��p��AU"6Xr��FW�]y�S�+!��GE�vL�;���r|p�O��~U*�=���=|�ߟ"�W��!���&��חym��cBcn����%�Z��Y���92)�A�u���v���a������F�����ܕ��?��hX�:���ӹٻ��@/h����}A?>9�><%}��~r����Y�:�=�������p�n9���(簮ӓ��q|=�2��[��/��4��s5׼2�q��pF칯��|��~�=��qtҬ=�K���y&��q} b�+��_�4����W���z]�'�sҬم�O;WՍ�O˺�u�0�������'�G�3�I{�����1I���B������sJ�n�;_���C���}��|��q�>1s��d��YS��h	�Io�ZK�+�A����8|��N����(kq0R�c�?e�q��o*�[����(��fz})�4�y���/�Z�\���qq���nA�v���u�׀��zR7J� �mq?��_E���]��$�h<=���[s|r1�;2a��| 6�e7��_x\V�ǟL�A{l���� ������b(�s����I2�]����Ox�W��bw�H�G�}	f&���8����������L8�� �Cf_r뼍:�H~�g��	��Jd����)���q+eͺϪ�&�/�	c�|T���ݞgǸ��J����g9~K�f�a�R�����o������r-�X��b?��+��\c����D;���?�nE�A�{�K���x���ۄ_9��z���:��k$���Zg>���ϛ&�&yV8<�ԙ����2�G~���b1�\�=f��O[��y�7ς�*��+���e���Rf�'��2�Χ������x"ӈ2���E��̹�Y��O�'^���C��<�"���.�:2N�_�^k�іOy�M�L?(U�eZ��K�cw�Ak��wj@��^P?jc	C�ϯm��Y�<+���s:V�=?����h? 5�k�e���|B7x�GU?�C<
���1�����z��U*�B�)���ֱ��gL��WByX��rW:N�Լ����>���^�5��5:�����{Ƨ_��0��N�@97��k��O�k�8�?��'?��<'����Q��v�G�/þF�����~�W���5[���zu]z��;Q��q���ˬ��z���]�ЛiR��~�ܰ*�M��9��hg�81_weJi�����Pb<�0˾�I�ܓ�M|�x��9����O�f//�<�}X�h�B��-��\<.�	l=�!��]�&�p����aFa-j��k�� ��^�9=f��}�E���\���0^c��[�o&���8nlk�^2n�:P���o���kz�dzz�0�qt�Im�E�ɑ�k'Rzj����iڍ��~>W�Xg��x��mK�z�A��[�o���i/91���Yîl$�v_"S�vg�#�=f�?����~�` �K�co.Ʊ3����ݙ��gIO�^7'�^�}��^�WLx�^����D�1n{�/g/�նX"}j��`V�'�"1Z~�q�k/[������ǅL�V?��~y�٘�O�Ƅ���8��13>��"ٿ[���	�#��b&�'F���ψ\�\#��#������q�8f����8��o[Y�#2�Jr_&����FB�2CzM<�%���j��<��Of+��-�GR��I���_��J1^x�����ÿ�Xߧc
�Լȱ̇�g\l^Cw�X|���0���|/�?P+�WH�|��̍�5Fy�8f�k��s��wr��1��_4�42��k��Z�y�Kl�W����e��5�� dѠ��_Ҳ�Yh_(���,������
mg|A�b�����~��c&>BY\U ��E�[��6���i#ؿ������VUwz.�?f]�m��EŻul�{�ec�F`L��Z��ܥc�x�/,�r�b?��v�kWȌ���s�ȳF���zD�����ent̥`nN���MB�fo�3�q���2��|��F����� WU������P|@�A/5���$/5,�%�>5v�5���h��� (�(����<�,���2-��u;��S:���:��.3K-����;�BbM����{��n@׵Cgx��{���������=��w�^�>)��O��X�g2�0�.t�M��>�Ga�(L�G�Z
s+�����x�S�,�|oz�;�=fT�=eg;��L[��׀�쇴�����Ӛ$��_&�{�8��J��W��k=�)F�Z(L�G��&��'^���>��YM|/`z��ǿ
���V��Z��F��/l�a��GrN�ʹw٨\����S)���@ˁ�p�&���4d�5��$�]�=�d���^M�$� e��	���F�5J��O��l��A�${Ɏ�l���Q��Q�k��.\�o�� \ς+�,�\��i�w��s�8SK?�A������3���#r��G:�H��Qw����ܗ�O��u$?��w��"��$?F�#�|w=��O��7>����^�A���ui_Oi�������Rk�����&�v�~6�WD���m����z�6�yxQ��^�}��F(o��Fq�n-�gv�`�c%��h�96B�8��1hlT��Gh.\� ���Tx�����>ZO�Kc�7G��?�ٶ)ɏj�H~����5$�_A��5��4�/�y��K���=��Ws'_oYc7/�B>w�(ӭ���Ɗ�8Z5�He���哮Sĥq?��-"�Yć��3\=��}��j�jn��Oꩯ{���:�I�:w�3�k����������l�zg�e��~w�����@�1-��R\�|Eu~f������l������F�(���51�sD✨8��8k�8��Yt|��>�C����w��r��1]����Ƿ o���hi�wũ27�Ĉ��N����/-���~ ���g�B�=���t����=�[fIb��\��3.?�\���4Ĕ��t��P��Z�L��AVо�dJ[����Y�_8'�l����d�݌�KZy,�v��r��� ��/ }���q̤�9��`y"���g���c��}��4E�K�9������|Z���x�_��$�7c��߹�d1�[&��#x�C߃~\XJ2g(�)ү�t��ǽ�q9Vk�h�&���?�uţ���^p���:��Gy����l�z	�Ɂ�C4���\k����u�w���z���i���4�vR2�ύ��7��5O?vd�I�[>�/�U1�}��O�>d#�W��oW��^��О ������ b���[�yϑj�w��߀�R�_~��Ri�A��ͧ�s8ƒ~�n���wkD��y�m�,���-����Jgx�m�z2��h0�����N�m=$�?׋���e0�L��ݖ[��o�w�	�neYw�hl�Ux���ֺ���<ʇ�sr����(������-ƕ�c��ؽv�|�L����y��|��'�2�d\	� ��TƓ�Y�ݘ�x]�J�S\�8:<%�<s�y�؋\|>��w`��{��������rL�����	S����(�U��x�C��>{����әa=O�1�a�4�?,u�3�p��Ɲ,߫��Us�#;��*���b�+Vki;�� Ph�U�)�po���	�Mb=�0U�e��Ѱ|��31׏�x2Z��XK�������z!cG{2��_���{���W�q�����y�u�ǆ�7!0@:�E��Z�hҾ��a�g�>;�����1��o��~����~�cc��;��r�s|M�~��#��dVR��6��%wdk�u�?�4��!���Uھ~[x=��|�u�;ٲ�E��NZȰ�s�}���ߖ�u?��2��|>7Y:79_�Yp��pl"<�bw�	Ϸ�s��
��'�Y�ryЅ���k{���6vcg{���T�8�cwt�wk�wX�ݚH�9G������.��I<�#5@:����G���6�����D�z����Ώ��eX�r�7�/^��7K7�w"���ˡA<�vK;BU��6J�@djH46�%$�W�%�ˣn�^͑�N���D��ܕT����E��<[����:�;�lP�Y<הՄ6'E>�f����N���'y�Mf� }R��V˻ۚ�D��/�?)~,�q�r��2�����N���x�7�'��P�����d����0����.y,Q���Hty�]:6�ѱ�#]d�8q`32�M1���� 8dgrCgsG��s�WP/!}MX�@�XT*��KnHtv�cz�)��e��6�Y�<[h�㗚�S��'�E3��$|1"wRD~l����êd�v���=�|(��(E��n�ѫ��Nw'�X+D�c$�&hDHCOk}�f�9)K���1J�7�
��!�8�h>���z�v��|s#h��tua�%��흐��P�B��N^)F�{����
x���� 6" �����$V��t��c�0�Nحݠ(d�m�����s�R}l�����W��J�g������4B2��F[ ��oiiD3j`�j��h�oKP��������Ϊ�`5�mh��TIƮ��[�5˖,[y�2�X�,^s/k٬�D�]G���9��IK�"���̣�Ƒc��[d͒��_����΍�7(w3��[���D��K�㌵%����)Z�^7߇��9�iv:y�L;�n����D�����w$p:���OE�q�6+�����<Nc{�+;�~R���zƱծM��3��i۲Q;yF�؅QYl2y��JK{�C��)�a��Sq8�i<"�h	��{v)Ȼ�k�}}�j47e�&�6�:�U>,M4�w�t]x�8O���ik���qhg<U*�[�Z�������=�a�C�
ᕖU�\\�jp+���XV���i �ԣ-1<��v���aC�W�۠ű-�������<i�m�a�3/��D�1�{�V_��1�S�}p2�|�sL��	���5�9����=��O�غ�3�7f�tږ%��\?�4*\
�\�>p����Wn�>p����g/�`�����]�6ࡎ���	\�pG��Ws�඀�n�Y\��p��Nl��n�^w�h��\|w�2����E�➸�X�9�"��Kw۞���M0w���˘6��!��_�v��2�k��h�p�N�Ep��]y���X�����,�u�_�(�8�Q����-��dj�3�r1q�.r���r�ɫ��X	ݼ7>�Q�Y.&"�u���x�����D�1:���Y�� m+�Cc�C��kݭ�M�.L|4��,��5:/L�?����G��Kh|�^��ܐ-�!��:/�����4��t$��!}���͡MDtAy�,�Ǒqo>�}r��/8�K��u|�q��5>�e51��5��^�\LDa�;.�p4>,�74y8�6ty��A��1_�i،�'����6*��Z�!_6��,n������oߛk�1e�E�yc"�6�F���D5�����Dđ�η�oL�)>��x�٦My&&�H�u�:�~%߂j�v����|�e=���u7��0�]g���ڏ�DE�_��	�/;��d嗘����/1u�����D�.T~����0;~9:V6M&�5���Jv��Kk�a�/�����U����=�	Qs!���l�*LD�0K�_&�^�
�P域l�V;��zӢ���vX�U~�͔��"�P���G��*�3�w��}��o����G{���r�7M�����+$F�3߿B� 4��#W�����ț$�;�&Y?��-<�Wk~�����ɠ�}�N/���{���C�~֠.���8}p&������p�c��;���~�^4�ӫfq�Q�~ �����gs�����o.�o��ӷ^��}Ŝ����w|����%�9���4Y�G#����B����]�����?��-K=�=Z{C�vÿ���?���r�~h�Eu��Gz�FN�1��z�A�2�}�ߠ�K8����ߜ���8����KJ9}Ԡ���~>��e��W����9����S�^[���+9�e��,���=��ӫ�8����?4�g��[8�x����%�r���9}��~����Wsz�bN�-��M������P����O�zUt���hC.��m6ƕ�c���o�o���W���q��o���������ϳ�|��1S[}�x�Qw�w��'��E~i��>��4�O����
#�}#1Dt���q���(���5�F���W!�4�U!�i�c����!�_a��20Bj��0BZ��4�x��1B�̠���!�]a����1B�V>I��F�jЮ0f!�����Y�����!O8Fȳ^�I8F��#�o��8Fȿ�'��9�!x�Y�)00B��J#�� ��;�1B69F�#�9FH_�c��0�1BP�
#� ��]aD�'�'�p���0BJr8FH,�c�ܑ�1B���!�$Oa���9#���������V����s��
#d��
#$7�1B��x~���)q}n&~�r'�����!�!����I�8F��C#d��k!��������B#����r����!�r9F��\��.���7s9FHg.�y��S!/�r��������_a����1BР��r��F���W�uZ�c�̥5
�r����0��sy�ؽ��u��~�[Y^^.�ћĵ��R^D嵜�G_�X���v�s�����b�lU��i�%	~<^�@Cru�w=�6+!�0T�b¬�����T*�B�@z`qJ��(%z���e��-��&. l �\����� ��<*N�� ��:�-F%<|6�L6���Z�#� ���/,�o�< O8/X�x�عF&0���g(�& ��a,��Q����q:�?f��B�8H/�c;���.�՜�Ҭ@w<��v�*e\8g�GG�рL$�^�Pʂ�V.< F4���y�쬁��B�4��CBDyaiX@b�D ��frA�$�NV�5�<�}$��G����a Yx�¥������a8D�d�hR:&�x�i����B2*C����/1�0��]���@��K�7�3�1�7��?��*�c.�s��^XYv����7��\9���5���{�tb��Ug;��u�-9�y/������?�N.���mѷ���k�q=��q�
�jp^�K��;��)n3h[�����L퍪-Q�����N�?ꇻ����s�+:�T�m�>������oQH���G��z�^�eeaԾM�ď��mI����q�k �h>�Z����2��w&��S�DS�+��>��?�'z��o�KWx��z�/��K��X���!#��<5�K�H�,C���a�~cq��랳L��������1Z��6��,��1��'F|&�����G���~A���$�$aPw_w�&6҆�0ޤP	�R�Ed������V.�7Դ�8�ؚhol����׊G�`�)�76(H�7*dJ�ˇ���l<?��Xr��9>��O���sK{����ƛ[F�D���mH���(\��`x��t�P�N�$�هx�A\���S�l^�SڋW\�II��S�lJ▽���eR�l %����+tS��PJ��*�a�B�GR�L?w^9^V�YK�e���S����ڽ��j��>��[���'�����v���h����A�~@��φqt�pv�=#��/�qt��8�Af�%׹�/�q4�urll,n�2��*��3h�2������ϕm�BkFfט�w���Ѷ��k��"��e�db�C>�3�/�%��L��x����U�M���Fό�SO�K��$���6 '���B׀�d�@��d���=����-i��T�%��3�6egd|�mC���D�����?�d.�JQ���?V7��]��Q�O�d0�q*����A�E����K�K�~y��n����G䇸�Z������Q�1�la�1)ld�(Yh?��-l��l(�u�.�㱱=�j�z��n�?����7C�V��*(�EPv�PvK�����ZHW,�j�w��G�Π�ʫ]�T)����^ƪ�{�P@��;���g�?�[�X�ၝ�L�+�n�A�����������~r;ȁ���g2A��ST�P �(��z0�q��x�B� �z���(E�����rN�nXz�p������D�lZ�X��p(3��
���6�Č0�gz�@��
ȷ��:�H��9S���D%ϛ�~*m��U�ӷ�2;}�ޫ�y}Z8�w=�ݝ �Ei���NSu%O�f-M�A��L'Ё�$�N���*�G��Oݾ���4@�WŸ�/�(����2�e�5��_��?ɇvfd��j��S�e�e�F^ʬGߢ/�X����]a�W�:S�á��\ߙ��C���(�;�Ӕ.�:e��S��7�̦
�W���uT�93!=�/1�c{��2���'c�����Gʹ�����c0�z�@9ɴ�{ �ѽ��w�/{WVU��Ͻ �^D�&� �" !�(���)�j"&(
��Z������˵UӔ�n���-�Zj���˦��Lԥ$����?3sΜsϽ`����}��3sf���?��7$M���/�Q�9�w��,��� �����d���H��o��6�������Yo�NQ����32����L���t��;�61�����&6�f���
��O�K�$Io�'�zi�,ic�?�G�48IZI��+��W���D���ǆG뉤��1\>���1<�����Ų�蘮�����m���}i<�8�A�Ǡ�7F6������όK� x���9���zR�#;��� }5z~��h刔��#u �ϑj~Yz6��; �q�L��r��O�K?�=�b� ���9[!�f��M^d~�^�_#Sf.�����EH=��O�?�kWz6B�L��	��2  lp�%@4X�9	��(�S�����E�p�Yv�� 1cF�3�c��_<�FˊɅS�ٙ��"�Y���j�t����,��z2��-��L��4'P8�)������.�l6��֔������ �g�=��d�ְ�T�[W��}�|����6�u0��o]�	��:���)�|뉝�|k�N�o�rT���~ ��;ܛR���UH�H t���t?�LC��?7��|z>��>��>�\���\��R~.��_/?��υ&h���!}3��C�|z����#}��2�����fJ�"'���rY�\A�Զ9�0�����c9�0��f�U���8
J�����8*F�9'�mP9�mN.�eP�qr��*�v�9��F@5;[��rr�퀲���}I�PY�c9���T{�\���,'�Q+�XN�.�o�N����={)sm�1��<������z��s�&W�|��[Ɖ\[�VՊ�����w��~O�;u�X�/�M�k��̟�M�k{Q�oke�m��_Ee��m�?��2׶�JʵG����?7�xi���x���)�:�6u����o��Oε�WQ;��B��b�K'jǥ<Q�cC�`Ǔ�
���g��v̵��L��o˔k�+�qͤ�>)�v{!�c��A��/�;��
v̵-�D�8�&��#j�l�v��l9Rn-�/�%�v�����=BX�`���0P������0����4)�����`��(�7Q&�.�ߒ ��͕WB��[k�v�[[&�L~3cn�v!<̭�.��̭;�I��M�&A�*p�D�Z��&?��Ҽ�����f�����tW��;���0���:a\���!�����p�������p��T��R��Ub��\/q���^I��S;��P�I
S���/;�U��*)�d'q�ؓ�TR�ɇLxPF'd�/UR��7ĝbOnɰ ��sN"��ݜ�b�'�{��b?"d��@:{���ƗL£؍lb�ؓWe�mdH���v�N�'FY|w�=9��bO����[Y|�Sy	���N*?�$ؓ�;�l8f�gf�g6���o��{0��y��xs7k�3�������uw͍P&E(2���B%*�|�%�UPP��C�<h����]bȟ�8".G
Bb�<�,S��D� ŐD��B�W	X���X^���Dl�/��>H?k���ٙ3�[ �v�C�����PN��#�!��>����d�:G�>�7�+��?ۆ�������mX�-��܀��;Й:���i�!aVޕ���x^81ӣC K{�0��=7L��� 0����#=`��]oüc��М%�����!�ST�o&0�)"�J�7`�VRtu��"�����C�Q^.�����ugn��.��amB�a"s��pɰћڝ�V���H��!��!�6�p�a�&�"v�s��b5=���|�T�Fb�D���b�O���*��`.�"�+�Q޹3n�^����~y��$y���!�_O�cv沑�ˤ7�w�	o���,F��?^&��L�sHO�1��?P���=D^0��Q#i%���I�O��E?��l���\�^��d�/A�N#�W�k�6v��K��yƜ��t���S�0닛�5/(�"���egd�\�e�[���S*��5/s�S���?�-���[4
i^2�Z0��\0G;=�݂��E�gT3�-`���+��`V1�f�li�|~���7O$�|�S�m��HwC�t�~��:Ӕ"�K-��4Б��QǘU�qˋ�0nyY�-瀎�����5E��:�@G��&�I1��V��1�F>�z�ө�����H�Ƙu�ٗ1k�;c�0fG��1�;?��&�|�ǎa��,r�F��������F�K~O־��En}c�Gs��Q=������Dl�N��W�#]׋O��/�U����z�EW|G���
�c���R��.�1K�����a�sv��^Adv�Eչ"�a��7��ҫ<���������Xi;]�YkW��V;#9z�cR����=f�R��n6�
i�Ѝ�}������(>�.�ެ��ܵp��(�w��^0����rH.��V����4�%n�S������ �n������]�������q4�8��4�ZH�?�z�_���������*X�K���}Q������Q̱?.��0$ I`�t$�WP�Eh=	�j%���׮��v�a$�a����ݙ:�\�x<�;)#������o ��ن_Z��?2C�>H����<�}uG�OG�x��Dj��sw�9ʷZ�9�&�������F�����ĸQ��Au��7g	qn��3�iW��w����NF>�"W��:��/ˉ	�<,].�q5�/�_��c��~��>f�>j6����s������g�ԇ���O��m��z�s��h^Fpl��zU��}&ԫ�Ǡ�\%ԩ�D&L5,#P�up*�#�Nbel�Y�x?ڏ�,��4��!H��}��!_+u(��s���幯B}�@��/T�M:��*��H���C�3�	��3}>�y��3���	���͗��	T�P��|�*�}�<jP�j`Z/�����O�ۜ��=��Q�\Ր~ꬆ��~<�z��_�ǽ�l���ŝ�G��e�񛘷�O�dGᵴ����F�����A}l�A��ڏt���da\�/�q��q���w&e�%� ��3���#~LY�dV�q�ȇ�_�v�]i�ܥB4f��uG��}L��(��Rw����@?�#�_�tD^;+�ڑ��y��3���w� Ԏ��޾Xn���Z*��V�f����2rGe��m�e�γa�#�9��:ܧE���i�f���mp�0�x�Bl��a�0>�M��Wٻ�dwddW��;��GT��C���/�/���v~}�j;��u�QVG;1mJ��Q~.������-mvL��uMa�☣]ab��:���*��vDf{+2�l��Fd�6b��Fx]s�VQv�'(�)�4�[I�a����Q���V:�s`T��{͙s��{���v��H�uǁݛN�?wnm��ӱ~)�.!g��ΆG���8�#���1��
��k�0�@���ƳS>޼%!{]�B=^|ůf�3zh&����S/,iiIFլiY���[��n���;�x� �͈�A��]lx}B���)y�S�>�W��C��}�by���K��n�ڻt��.i������2nV�'�x�����������'��e_�9<��oN��%;ZnE.o�a�s��Q������+�˯-���L���n��'�;���3���pn���'��cC��9�{������pգ�תv݊����T�I����,6LX6��m�7��f��X8'�r¼�4�HV�T��kt�4�)�&���w��.<;CW<�l�䧺v��a��-'vОO��s۠�Ϙv�w���4���u�7n�{/Яt��д#5��nu���ͩ����Lݱc�_�K�n�Ϋ��e_~m`��#�ݾ>l��f����'���n���<�Tq�����O��[�9��Uy�.^���#�&Zjn�\f���*=�����'Ƨmv|)w�ɗ�+]��h�]���~|��xlXh^��~cG�]�=7|���ˑ+'=;5��5��k�J��zM�5�#���� ��ſ)�3y��snvo�>�9��gsSB�zh��v�T\��7��[B��bo���^z�i�_i��f��Pṯ��C��՞�359����#�����]���������Ƥ�/�q�d����`E�����j	׭���+��^zq�O$��_~�d��Fgo��;hMi���Y�����n+X;u�������81����1���e��~]�n���zl�ʥ���^�-�3��¶.y�O��V����p|����=�%Lv{5��]�^�/dx�z�v���.�</t��¡�'�͎#��T�����^�����0���Ǘ�w��a�G�x�}n�~�;��\�qi_����7�=����sI���������^Gv7�cHi�����6����>2-Y����-�����(y��?6�(t)5o����'������'5��[��*�^�]n75�/e�����		���QQ��D��1.M�/3<�;�}�Q��q��8`�ۃ쟱<�GS�q ��Q�	�c��ǑX��=�>6��X���~2|gcy�RG�q��Q��elc,��b~���}�]͒�A��]#�(+[��V�߈q{��x��x �Fk��k����^�F���MЂ���>��!E1,��7��-�q�����oD�l�7`\U��QP���o�8jg���'(-g���/(g���5�T�6���7�k����7�����j�߀���>#��!W{K,�%�j����K��c�0J���o�!iV�8����V<��o��+�/�NƆ'�o��fW_�xY�|��+�G멜��������odxv�����1��2���7��#�ю�7f?�1�F� 7�o@���1h�x�y�n��2��j{j�1����F;L�ذZ��	4���_���2���P�F�`�3]�����mԎ������;�2;�'Q;�����W��:�4�(_�(�S��R2���ǈ�V�o����0���4)���o\��{Y&���G�7}�o���*�����7"��G2�����!<��H³�o�Y�|���.�n(!?\���e�-c���G2~E�����"u/�&u�d�'e��wu���׈��|'-�k�X����f0h�����k��b���0v�.)��N�8^�Ý,�0g,�/T%����9���a��b�f���4�÷��O1|��N1|pW��+QI1|�UR_�,}Gd�S*)��Z�a�|�$�~�J�al��SK1|�j)f���w�>�T��T��j)�o�,�	j)��y,�o�����7[_�Z��[���?O-�����!{�2߃����X0��х�rɳa��.�P&vX^n�A
-䑈�y|"� bd$�4�sg#b�#*�/9� 1F�n�I�J,���8��9b�$�abp'Ft|$�jJQ��i�����3��,�#y�������D� p�i��� ��А+��С�!"�#����q_~r����?����n�/��?�[4�9-!�^�
�a����x78+2��W-��8�*xV0���Q�,����j�V,�?S�*=�)嗬v����v��#��<pRV`�尢n��O0Q*oS�z��>��>3��%~���⡫W��+���S�>��^v!y�KW=����^'���w:��Sd��9�F�Ht�Ww�Z�K�i�{#k�=�pg�՗���b�kC.�'��bZ��{�܀��2g\4(��E�1�����Y�q��U�����Ϟ�_��aZ]�+޳vF��8��X.�kqi]�엉����.� i��p���\z��Ëɥ�G%B���Oo���<��3��'��߇�AK� jE���������'σ�>�����$}�칶���� !/�V�IY��݀��%7�vqFT.p����F��AG�����&Б�VЁ� :j��Q��:j�{AG�w ��R��
�@eL�6��9ۜ1s��N3s�9c�M���1W3�ϐ٧���3.�kv\�
g�j�l\@��+���J��+�/�͍�JCm� Z�g������+�7�������;(����l��k�]�r%�+�Y��'\x6���g=�Q�4��+�L�;����[����Ԑ�=]�����G��*\�p�x�	�$>���P|�S
X\��q=>5X��S��ŗO�����q_"���%y���
���Y�/I��}!J��O{�#�u�bX��P��q�8�u����p���r�������ݮYv7�{�i�-����icZ[�R���֑Pu��{UqBڠ���Z�izg�9��#����}�{o����;��<�|;oޛy�}������Y�	9�L�A�6�������ψ�1���e�a������;���'"�0��?����)��d���!�����p`�c�^�OJ�{O_���/Oh������I_~tr5���oò�ob���N"��m}Ϸwd��.����n��r�����a�����'?������]�m:m>�9}S�5Ԏm)�g�~&~b�\.χ�i�{3Y�غU�O1^i��~mU�k0�?�;�e�� [U�j{��m;z��^&�v�x��~Uf�}<���oS��="k5�����]h�#ϱ���!C �lH�9�W�z����積_��!����!��kW��S��t}m��(�C����a;#2:���/���ϼXۡ��&-خ֓q�E��"l��B̿f��x�=/���=^��E�	^��+/�e�΋`�{y�=�A�Ƌ0Q_㷡���[z*���޻���ء���~^�-�wk�k�\)ٱ�ɭ����fH��������������!t�l��ŵ!��|�n_WA���H>���N�&�W���,>���8*�Ft͚G��ї|
���Q�|��4�O�6xt>�$2^9z��^$��m����j��z5�;����|knG~���ņ>�|�}�ߠf�E��lY�Χ���_@�ʫ���-��Q]7|��{N�:�НO9�Y� ���r��e��J��{-��8�}U	�(J�ܹ�3�ű�<��?�I��zCe��kO�㥣e���Y͏����~�?)�am�k���]��ٌ����_5�c�1��7��O��w�k��մ�Ѧ�{��o�bw��G���9����9��n��F�;ǲ��5�s,��v�m��.�:9�W�_�%^Y��x";�
��O7��x�5r,X_j�űl1e
 e��s$Si���d*ur� 9�˦L�k�L�vÔiFSΝI��S&��)��ʹFɱ\3e2�˹:ɱ�L�J�a1n�ː)��QS�eؔ���K�Fbc�L�S�u��./wȏ8�s�^���w!eMr�̹s�&8������!���/Z��}�8;�r�����떚�7�r5{���>����׼���vnԬq���ݎ��+�4�?��~R�׬�C��yEƑ�I����K���(2�]6�[��^d����M8����m��s\I_��G|��,���������cv�{�[�8�G���v�o+�3���Y�&95?�������u%=}��� �tK�t_ka1��"���"##�#���uTq~;OK ��v�r1��Ҟв��VrP��;��m�n;��R�!
��2r.���u���:�ƿ����ݖ�\��&~��m) �s���C�Z�
r,��c�����{�9��:����W}�������C���|v���>���+��_u���>��YM���W �\���7}b�[|�������z�L�]���9Ox\�4�9ג�房�̙[D}���G�0�!*��k�r���%����4}Iˑ��4��R]�c[��}��o�Ε�v*ۂ]�Cf2�uNͯ��S��ڡ��LY ՚�&���+��f6�G��`�'6� L����Q����0��\�h$���z�r�G��_/{Н�8]�3'�����:T��~���۰�v�c��w�!��^z�{�#F�8"xD���}�,a�{��N2$�x�9������}Q�~��E��GP�Q����=#V������2ꚬL��_�!8�F�����	�>�M=��m5�N�|J�/�g9(�Q�f)\67Ym���<
����u:�8���q�zٷi�G۶[ϯp��s#V_�>.�з���p\!�����/��)ߏs=WG���˩�ǿ���o�I��G�5�5G|�.��{�/�Q:��K3FdzO�Nfՠ����ZS~�/ɽ����#�q�ŲL�E���g�)	�@3Wr!S�)��N���ˑMםU�Hɥ�^ŧi
�"%ɯ��M�V��2�:Σ�mY�l;�+�_��H��%<��_�3To�*�o$��{x��'�����x���ĳ|�g���5�r�P���r;�����#�=�{��P�(�;��-���z�rm(׏)�L�^�\7+��kM�����|T#��~�-PO]]�9Q#s^02{�Q�Q��ii�9#s��
�-pEe�v���u����+�?�������q�%\K�=������oNlzht�O�N�ͽ�V������y>�����5�~��O_�������i�yלD�ו���ĵܷW[��W���I��ྸ>�N���~�yo"\��p���ԛ��Y��k��D�ז��}|͸��������A�>�_�x!H{+��~�s=g����:A��dD\���y�{��_dh� q@�~���-�����"�SU�o�����j����߬Sז�цg���y!�'��ǎ�v��(��0�����W��	Z/���_�ֽ��>������J��*�+�����O��4�]zI��^��� ��8f�3�<ӏnN�p�ب�JX��J�=���j^_��&���v�����?�S޽��i����z�o"�s�������x~H<��֐(W�~�͛������znj�������Ϟ��j<O�r�1��w�;Q�Q�_#�ş޴x��Lb�dЧ@y\�p������ݼu�k�*�]rК^�����Mu�����J��~�z� �Zp�,�e�#�����俬=���ȁ�sܫfl'A��5n��T���#*��Ɠ��ō����cVCω3�!�_�c�Oc��I*76��΍�i\�ZpcO����X#�_���Tn��������ϩ��u��X���ō7X�������Ԃ}�;̒99�n1��l�Y�#�0BWOx�Ǒ�qW�˼�#���rd�{
G�*8�#��7~��/���.��QG�K�}�#��8��}7��j��M�92l��Mw��ʑa���S�u�Ȱ-?��:G�}����T�Ȱ�pn�)*qd�;F�u�T����pq���ɑ�\w �:Z0=�������� �/���rd|���:G�s��U��h>�:G�}��2�T��Ε���fqd4�^�#�>�X��zH*G��t��sd8Vn-��srd�����j�Ȯ6�Ƒ]^RGv}ImYjim�٥�qd���6�,���#����qd��jڦ���)G����L�J�KH��)�E�uS�T;g�d�s�#�6e��ǤL�*m�#�d�đɹwɑ�a�T�ϊq����L��1�L�Z�qS����d12k�2qdͦ|wpd�"�$�������s�F�ָ�r����dzH�l�rߍ#��Y����\T�Ǎ#��'9�Î�?�x�S���S�8�13<����q0qd�9���EEF��qdg�Z�ɑa}���Α-T���o�K���@��#����K���S/��?v���~�;<a�V����k0K}D��}͞_?�,�B��k3�}K�Gt%�@6tK?%gf�1g�[�9��ȸ_�,�șយ8���+��}�N���S��^{u�~ɝa["������W�Qh�q���J|������v[-�hU[-�EU[�:���x��n|v���>���g�ݮy����ڧ}v[.�{��N1��H�+️��j��=�ݶ��j������#��|T�O�(�K>����Vm������g����8�����Lɟ�����3��_�M�b�5@F��,_k���@��]?�a�G�7���[��펮��A����i�:�6G�[!�O&J�l���6���#�|�šn+��}UV���\1:�ԟm�B)�#]��CC��\e���q*�WJ������^g_��[H�K1K���6y?�y6��4±`$�G��S���_!���E��ߣ�}fο�ㅣ�[�2�[��Ɏ��m=��.���Y�g�`gY�#�XJD'�`�c]F�+���}�}���\���㏳�|��1 e~����S�"�"�
�Y�J%�_�J�R,�K�?��%ϫ���z�����S;!���Yơ�n��R��p��ǂQO���V����D1�_�_I�+����ʊ���T�]Y^s!tP��������x�Խyצ���*�*�޺s}�ڲ}�f[9m!��~����l����B�8���:�=�����k�%qa�y�Gd�l~H�6�o3�R�nOX>�	���������؁�VJ�O�;�Je ;Ĭ����U��d��R�����{�XOLJ1;���H`qӟ7o�gٓ۟������?��A�b�z��$�`�^��P��m��d -��>�/�I-��}�Hn��&�X.Y>W,�r���
��ի�g�j�����`�z`e\��߈Y�?Y���^�_�Z��쾧j�7*���A��V�����]�d`�3�����<R26�KY*YHbJ)��Y��\1+�9��j���|2�t�˘ʮZ�/��?dD�!O���\�_��P70�_J������D5^x�=	���r��jzN�ww���}d�?KӨ�WPA�K����i�aG�P"[�� |Ov��bz �(��eo��M�Zȭ�d�QD}�l�Ր�^�������ׅ����������b�@0�� �J8���uq۠N�l� F���+ht�T�����?����?��oQ�����O�������}�*�h.��XY���>��/�EI�O���fm�uM3�L'Xq�P��(���˩�K���(��/2{���ۑ���ɳ����sh�����(�����p<d����]����������w�,���'~�����a`ʓ�d�3�)�H�+
�l.����f���`��d�?���\v�#]p�ƙMb��L�����$�Y;���:I�V��Z�3YΕ�2����R�7��	I�,�G�f벻7�m�w��3�w�/�U��sem�T2�RB��%hg����#����qwG��h���Og2�L��Ih�hOh�{�����9���o��8��e�?
���k���fo��[T�<SW��ρ����C!î�h���ߺ�[��μ��]k�왁=g���}��`0��?C;��uq�S�d���P�������b����7n�#�P��bqO���<�������>���� ����� �����q���7bAo��z�Y��pt1\co�@�e��C9��J;��`�їt�����I���`�\6s�ël��sb ��?e�1��p��4��������>�e��M���r�N�H��Z���3y����ߛ�aw��=۟zj�N�)�i�g��٣��/f��<��+1�����D�0yb�̫��/jނ87m��� X{����r�S;Y{F��iu��,n���"o�����%`��߈F¢��� ��p����W�ף��tw��Q�o� X��g�o��GBh��ż�_]�]����r�]�suwU�Y�n)�j�?�����b�`Dc�9�Ɗ�>�������V�Y�����������<�9�y�s���<�9�y�s���<�9����?/#r�                                                                                                                                                                                                                              simpltest.tar.gz                                                                                    0000666 0000000 0000000 00000011557 11162434065 012746  0                                                                                                    ustar   root                            root                                                                                                                                                                                                                   � 58�I �]ys�F���ħ�%��S$r�[�X~q���%���n����5	pP������  Eˎ���"�>z��=3���zً�߄��K?N��@�����폝�۶��p0��}{0<= ����@�8q#BDaX����(����x���^D]oE���"&�E�:�{��#r��p�5{n⚂CC��>��a���#j��)�ʧ�Z����oؼ�G��{��4U�	��4٬�]/�$M�y��0���z��g���`l��N �8��t�Y99�c�����d���Yo��a@�C������=�!�p2LF#B��t>��yb͖�6kK�e�\��m�l��L ���s]�}�!��=�֮m�.~L�����	<�7&.�F>����F(GH8'ɂ��ū^��?�<rW�:�>�?�_<�3ᢇ�4�Z�?�ه�(��Ē�7`5]F�(���`K�'�nD�#��K�s-�$���Yě��GfPܔ��z~�����Hy4"I�ě�Z	H�z���:D���ILb\~ ���P�ҧW��co�0\�d�^A!V���/�ĝ�h�S8l,��lb
�~��P�|�&�1d%��K"Jq���@A0�1��q(�#w���%�8�^���].�kV�-\����W�ϕ���T�0���Pςy`'z,�����>`����/�<�`<,.��㸞���1ŎMX�V��������b!�M���R�]�^�bY?.h@�� ��$��������L�4d�'F���o^�Yi4F�CGl)2=������Ĳ�ҜFL��瑁)N�S�B	�Ȱ4J!S���H�LqC����#�v�v�S؜�	�3�3��$�dE@��d��kwk��o���$�gu����lF�M�-�$���H�+?
�����l�Π=l�[g��A��o.�&�s�<]�B��b�QV�V5�61�NPF���T���Z����b�HMy2�L%��0 nTEjP
�P��k)hF���)�\�{
�+���,6��f�X�4+S�0de�,��7K6�S�8`��`v��{�	��h�Y�Lj�	� �2��A���G���g}
,6�n<�)Nk��:&N�M��2��J�c��0��#��嬤�rPCl9�����X��&�1�e��_����|�	��n(h6Ք��1HLo�P�b��b�Ɇ��7�Xd�OA� �w�����~��$�"�|6��~"���f�6Sʑ�k5�{4����&��MC�^`]�wU!��h���m��P��[*��Io�F�3���3p�aIj�J��pX��>�8|s�|�5��eڷ����M4XT�Q̅V`��	6��\1���_�(Y��rn��&Σp��d7��cV�������%'���F���H� ��᨜�>�zj�XDM�c��ų'�q�����`z�w���ղZ�\�D2W�W�e��.��I�0<�Z��O�L��I�xMg�ܟ��"��J'�C�H"6h:~��$�!�0���66D�z�f��~�9�U�~:����e��3 ���U=�7d�e($�ȸCo$Ąް]w��l�E.A�BB��T�t�1C ����_'���V.���>H�/�����ߝ�԰Tr�lH��s��9�Y��.��2(�`*`�U�L� ����E�^�.��]y]n$��0�9���i��
0&s�GO[:ӫ쏍<���S3�Ȝ#5DG�*&h!�N\�T�1#@�ׂ��q[�XI.���Ī��nj�
��[O����:�r	�bB[� Q���(�Z���*[�$��)n'�$m�^��!�'@쩡�̆��#ӓ�@���VRV��˩�A�LXլ���b��S�A"�Z=p����L_,�bW,�.������C���r���/�xD%,v����bʧ
�z����!q��f�4�n"�W)��OW N��@S�bz��3x2&-7����g5��經�d�����Fe�_��>���=�W�������s��Co���/���!�fH)�e��^�r�gn�N�Cs҃��!|�8�� ��� �g2
!����7��?}�?�G����g�_?}uvz��v��oN�l����]����#Ǻ����g���6t�I[%������Ӓm�Lj�lP�Z�*Ŀ~de+j(�'5:E�Ֆ�yʌq��/�l�>l�p�#*Gk��m��(�c��"2-4��<ѪI�}��'����pSe��"���M�9�挜�>�	�@`����eL�	m�P�`f
�٘ۑ�e�O�r$��H�gS_���Z��կ��fЫM��}��b�z���T�E2= �I����7��8�������A��4�����O�����.�%�:w�?��N�>!�x�����P��Ǆۉ��������
m��6�M���(ۆ���+��P�&�a��R�#Ɩ@y�^Hl{2M�������$q�!�S�*�71�������/��ѷ�>D�+d�<���
���&J�h���֎��1&[^'5�e��Q�� ��]b���w�M{o]�-*���߳����6�l�G����3lw��-/�ZL���{�;b)~b���s�N�����Z���픅��Hilq�A;�z��k�B4o�R�(�ȫ/�z!E�BB�4�����)�f�笺7Vk��p���p�JJ��
,����V��tM�j�H���F�b^@�m��'���s�-��]���dx��Y.��w���B�l.�R65�2�tIV>�Om�Ŵ�Hez{����?�ޢ �?Q��X�f#��0��-��R3��ՔF�+����g��vʓ���a�� �p�r�8'�!��7l���d��	����q�I42�k������@kX�lO���g4p#?��U~䷀�t���ʡ�sR��}dgj�j�_3�9�9�8|aj�;���-�eͼ�.�b��4<���2���1��`��0�rJ��4뽱Ed���v�N�L-Cy���85�+f���T�:�����abq�^h���)[d�*Y@���5n�Y����f�'����C+x�,<��2��V�ư��bq�,=pny.�m���l����6Ρ����=�����I�����u$g���РBx������1q�*��8 �cA�����u?ȳ���~����~���Ӻ�����g&�����Q����(�������^ȸ��l�u��O�eQ�8�w��� -nk��H�,���j�J�h�=+u��V�����t&O�m�TT�*�n�4"uV�%����X�����f� �>	�D��-�Y쎕)V��&6MKrgg@���d~a�uԝ��0A���_��W�����(h5��A��KzZ��;{C���-���z����\���4٠u��q���ʡF]ź��U��#�w��5>�t�:����O���/{<�翝F����?g��H��~�\�z[�|������
rk�k�E�!�Ӕ��T��������\S�=���Њkk���^_�g��56��ը���r�W���р|�a��)�l\��〴���q�k7`Öns�����_�^���	��ؑ7��ބ��!�޶�;��r�q%�ŭr6:�Q�Ʃmr�T���B��C e%d�I)i�|�����*eed�GYQ�/e%�gJ��m�O�|�A�K#��<M6��x�P��+I�Ovx��l�]�Q�^�P��#�FOr:ӣ��3�b�d ���c>�P��,iY1��
��@�L!���8d9�]�ͼ{���������k���B��џ`�����/�W�7~���ӏ�c몎�e����e��_]Z�O��K�w����z�1s��F�,#�/�H�/�����H"�dO;�B������n�[����ZgZ���6���av�|F�x8/K
��0��Ǌk�Ȓ>�B����,+� 0R
�1j ��	R�f i��G�4�ő�o��"����>��V�E�p�dyuJͰX��2,��G?lu�o0������BF��d���_<}����ʕ�>�>\=�~�����w�̡^��,�F���;���o����|5o��z$�x$�쵠B�����E�i�;:�:$v�(j�L,���ºh,S�^Иf�����Q݇�>ɮ��7�W!�}�Z[��2�F�0A�Bt�o�a =/�Co�0qM��@��-ofO�'�#	�=m[
92e�G���C��3�̮�t�$t�آTq�_������Ɠ4%;�¦c���	�ˢ���:��tt7�$��&�/�|1�D~�U�hk�G~͕�p@
	9������'��v�%�7dzˆ�"�Z�S�5V|M�FƐ�V/����G�J,b���/ q}Y����ܱyU��4p���`lOF�	��Ol���s/�Ëʙו/P����f�~��]���+�h�oB�B:'�����1�����h2L���|���<˲GX��@�}Ҭ���.�pu�:j���x�_�}{ܬ�� ��O���=`Ւ��kV;�����t��w��e丄os]�ͻ~/�mN1�@��0��G�@J�����#����,��
���n��m&���
�t���[�R��!� ?����?�r<'cO�~�7lK�;�>�x�����VG�����`���㽐q�g'�� Ϟ^<���p|��!w�I�᠖?? e�!E�N�Jy�:����'E�?j���B�����C,��R��\Zk7J�L�M8��I��%k��������(Yߕ��v�U��{���?��ǜ�I��ܭ2�]B��/�0÷��&J%�-�fnB��&1gJ�����@���j���j���j���j���j���j���j���j���j�wF�}4e� �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   