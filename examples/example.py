# -*- coding: utf-8 -*-
from quest_module import *

def isr_callback():
	pass

def QUEST_SOUND(path):
	return '/media/flash/СЧБ/{}.mp3'.format(path)

def QUEST_SECONDS(x):
	return (x * 1000)

def QUEST_MINUTES(x):
	return (x * 60000)

class PuzzleDummy(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Интро к загадке")
		self.setDuration(QUEST_MINUTES(10))

	def onRun(self):
		self.backgroundPlayer.play(QUEST_SOUND("Фон лес"))

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("Мышиная норка"))

class Puzzle1(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Пень 1")
		self._testPin = GPIO(414, GPIO.Type.IN, isr_callback)
		self._magnetPin = GPIO(406)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("Мышиная норка"))
		self._magnetPin.setValue(GPIO.Value.LOW)
		usleep(200000)
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onRun(self):
		pass

	def onTest(self):
		test = self._testPin.getValue() == GPIO.Value.LOW
		return test

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)
		self.backgroundPlayer.stop()

class Puzzle2(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Пень 2")
		self._testPin = GPIO(410, GPIO.Type.IN, isr_callback)
		self._magnetPin = GPIO(405)
		self._durationMs = QUEST_MINUTES(10)

	def onTest(self):
		test = self._testPin.getValue() == GPIO.Value.LOW
		return test

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("Мышиная норка"))
		self._magnetPin.setValue(GPIO.Value.LOW)
		usleep(200000)
		self._magnetPin.setValue(GPIO.Value.HIGH)

class Puzzle3(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Пень 3")
		self._testPin = GPIO(409, GPIO.Type.IN, isr_callback)
		self._magnetPin = GPIO(404)
		self.setDuration(QUEST_MINUTES(10))

	def onTest(self):
		return self._testPin.getValue() == GPIO.Value.LOW

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("Мышиная норка"))
		self._magnetPin.setValue(GPIO.Value.LOW)
		usleep(200000)
		self._magnetPin.setValue(GPIO.Value.HIGH)

class Puzzle4(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Пни 4, 5 и 6")
		self._testPin0 = GPIO(412, GPIO.Type.IN, isr_callback)
		self._testPin1 = GPIO(413, GPIO.Type.IN, isr_callback)
		self._testPin2 = GPIO(411, GPIO.Type.IN, isr_callback)
		self._magnetPin = GPIO(403)
		self.setDuration(QUEST_MINUTES(10))

	def onTest(self):
		test0 = self._testPin0.getValue() == GPIO.Value.LOW
		test1 = self._testPin1.getValue() == GPIO.Value.LOW
		test2 = self._testPin2.getValue() == GPIO.Value.LOW
		return test0 and test1 and test2

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("Мышиная норка"))
		self._magnetPin.setValue(GPIO.Value.LOW)
		usleep(200000)
		self._magnetPin.setValue(GPIO.Value.HIGH)

class Puzzle5(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Покормить филина")
		self._magnetPin = GPIO(415)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)
	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)
	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("дупло"), 50)
		self.onStop()
		self.backgroundPlayer.play(QUEST_SOUND("Звуки пещеры"))

class PuzzleVolcano(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Вулкан Эксперимент")
		self._magnetPin = GPIO(401)
		self.setDuration(QUEST_MINUTES(15))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("взрыв вулкана"), 40)
		self.onStop()

class PuzzleHoffs(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Хоффы")
		self._testPin0 =GPIO(402, GPIO.Type.IN, isr_callback)
		self._magnetPin = GPIO(400)
		self.setDuration(QUEST_MINUTES(15))

	def onTest(self):
		return self._testPin0.getValue() == GPIO.Value.LOW

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("тайник (скрип)"), 70)
		self.onStop()

class PuzzleExitFromCave(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Выход из пещеры")
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		mb = MODBUS.WriteRegister(1, 0, 1)  # close the door

	def onStop(self):
		mb = MODBUS.WriteRegister(1, 0, 0)  # open the door

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("отъезжает камень"), 70)
		self.backgroundPlayer.play(QUEST_SOUND("Неизвестный домик (фоновая)"), 40)
		self.onStop()

class PuzzleMirror(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Зеркало")
		self._magnetPin = GPIO(388)
		self.setDuration(QUEST_MINUTES(15))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		self._magnetPin.setValue(GPIO.Value.LOW)
		self.oneShotPlayer.play(QUEST_SOUND("Неизвестный домик (разбитое зеркало)"), 70)

class PuzzleLamp(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Фонарь на место")
		self._magnetPin = GPIO(387)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная палочка)"))
		self.onStop()

class PuzzleClock(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Шифр, время вспять")
		self._testPin0 = GPIO(392, GPIO.Type.IN, isr_callback)
		self._magnetPin = GPIO(386)
		self.setDuration(QUEST_MINUTES(10))

	def onTest(self):
		return self._testPin0.getValue() == GPIO.Value.LOW

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная палочка)"))
		self.onStop()

class PuzzleCandlestick(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Подсвечник на место")
		self._magnetPin = GPIO(393)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная палочка)"))
		self.onStop()

class PuzzleBook(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Достать книгу")
		self.setDuration(QUEST_MINUTES(10))

	def onSolve(self):
		self.backgroundPlayer.stop()
		self.oneShotPlayer.play(QUEST_SOUND("Шкатулка (открытие)"), 60)

class PuzzleMusicbox(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Музыкальная шкатулка")
		self._magnetPin = GPIO(385)
		self.setDuration(QUEST_MINUTES(15))

	def onRun(self):
		mb = MODBUS.WriteRegister(2, 1, 1) # enable music box

	def onTest(self):
		mb, data = MODBUS.ReadRegisters(2, 0)
		if(mb == 1 and data == 2):
			return True

		return False

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)
		mb = MODBUS.WriteRegister(2, 1, 0)
		mb = MODBUS.WriteRegister(2, 0, 0) # reset music box state

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("открытие тайника (волшебная палочка)"))
		self.backgroundPlayer.play(QUEST_SOUND("Неизвестный домик (фоновая)"), 40)
		self.onStop()

class PuzzleVase(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("Ваза на место")
		self._magnetPin = GPIO(384)
		self.setDuration(QUEST_MINUTES(10))

	def onArm(self):
		self._magnetPin.setValue(GPIO.Value.HIGH)

	def onStop(self):
		self._magnetPin.setValue(GPIO.Value.LOW)

	def onSolve(self):
		self.oneShotPlayer.play(QUEST_SOUND("ФАНФАРЫ ПОБЕДНЫЕ"))
		self.backgroundPlayer.stop()
		self.onStop()

class offPuzzle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self.setInstanceName("OffPuzzle")
		self._relay = GPIO(15)
		self._relay220V = GPIO(389)

	def onArm(self):
		self._relay.setValue(GPIO.Value.LOW)
		self._relay220V.setValue(GPIO.Value.HIGH)

	def onSolve(self):
		self._relay.setValue(GPIO.Value.HIGH)
		self._relay220V.setValue(GPIO.Value.LOW)

class nextPuzzle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self._nextKey = GPIO(407, GPIO.Type.IN, isr_callback)

	def onTest(self):
		test = self._nextKey.getValue() == GPIO.Value.LOW
		return test

class powerButtonPuzzle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self._powerKey = GPIO(391, GPIO.Type.IN, isr_callback)

	def onTest(self):
		test = self._powerKey.getValue() == GPIO.Value.LOW
		return test

class serviceLightPuzzle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self._lightGPIO = GPIO(390)

	def onArm(self):
		self._lightGPIO.setValue(GPIO.Value.HIGH)

	def onSolve(self):
		self._lightGPIO.setValue(GPIO.Value.LOW)

class serviceLightButtonPuzzle(PuzzleBase):
	def __init__(self):
		PuzzleBase.__init__(self)
		self._lightKey = GPIO(408, GPIO.Type.IN, isr_callback)

	def onTest(self):
		test = self._lightKey.getValue() == GPIO.Value.LOW
		return test

MODBUS = Modbus("/dev/ttyS1")

pd = PuzzleDummy()
add_puzzle(pd)

p1 = Puzzle1()
add_puzzle(p1)

p2 = Puzzle2()
add_puzzle(p2)

p3 = Puzzle3()
add_puzzle(p3)

p4 = Puzzle4()
add_puzzle(p4)

p5 = Puzzle5()
add_puzzle(p5)

pv = PuzzleVolcano()
add_puzzle(pv)

ph = PuzzleHoffs()
add_puzzle(ph)

pefc = PuzzleExitFromCave()
add_puzzle(pefc)

pm = PuzzleMirror()
add_puzzle(pm)

pl = PuzzleLamp()
add_puzzle(pl)

pc = PuzzleClock()
add_puzzle(pc)

pct = PuzzleCandlestick()
add_puzzle(pct)

pb = PuzzleBook()
add_puzzle(pb)

pmb = PuzzleMusicbox()
add_puzzle(pmb)

pvase = PuzzleVase()
add_puzzle(pvase)


s1 = offPuzzle()
s2 = nextPuzzle()
s3 = powerButtonPuzzle()
s4 = serviceLightPuzzle()
s5 = serviceLightButtonPuzzle()
set_special_puzzles(s1, s2, s3, s4, s5)
