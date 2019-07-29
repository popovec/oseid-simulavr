#! /bin/bash
#
# $Id: refmt.sh,v 1.5 2003/12/01 05:48:34 troth Exp $
#
# Please use this script to indent your C code before submitting patches.
#

set -x

OPTS="\
--no-tabs \
--no-blank-lines-after-declarations \
--blank-lines-after-procedures \
--no-blank-lines-after-commas \
--break-before-boolean-operator \
--braces-after-if-line \
--brace-indent0 \
--braces-after-struct-decl-line \
--no-comment-delimiters-on-blank-lines \
--dont-cuddle-else \
--else-endif-column1 \
--declaration-indentation2 \
--dont-format-first-column-comments \
--dont-format-comments \
--indent-level4 \
--parameter-indentation5 \
--continue-at-parentheses \
--space-after-procedure-calls \
--no-space-after-parentheses \
--procnames-start-lines \
--space-after-for \
--space-after-if \
--space-after-while \
--dont-star-comments \
--swallow-optional-blank-lines \
--no-space-after-casts \
--case-indentation4 \
--ignore-newlines \
--tab-size1 \
"

if [ "x${STATIC_TYPES}" != "x" ]
then
  TYPES="-T AvrClass -T BreakPt -T Irq -T AvrCore -T CallBack -T DevSuppDefn \
  -T EEProm -T Flash -T GdbComm_T -T IntVect -T IntVectTable -T Memory \
  -T Port -T PortA -T PortB -T PortC -T PortD -T PortE -T PortF -T SREG \
  -T GPWR -T ACSR -T MCUCR -T WDTCR -T RAMPZ -T SPIIntr_T -T SPI_T -T SRAM \
  -T StackPointer -T Stack -T HWStack -T MemStack -T Storage -T TimerIntr_T \
  -T Timer0_T -T OCReg16Def -T OCReg16_T -T Timer16Def -T Timer16_T -T DList \
  -T VDevice"
else
  # This is the default case.
  TYPES=$(grep typedef *.[ch] | awk '/struct/ {print "-T " $NF}' | sed 's/;//')
fi

indent -v ${OPTS} ${TYPES} $@
