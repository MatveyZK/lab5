#include "Account.h"
#include "Transaction.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class AccountMock: public Account{
public:
AccountMock(int id, int balance): Account(id, balance) {}
MOCK_METHOD(int, GetBalance, (), (const));
MOCK_METHOD(void, ChangeBalance, (int diff), ());
MOCK_METHOD(void, Lock, (), ());
MOCK_METHOD(void, Unlock, (), ());
};

TEST(Account, Test){
Account acc(1, 50);
EXPECT_EQ(acc.id(), 1)<<"Неверный индекс!";
EXPECT_EQ(acc.GetBalance(), 50)<<"Неверный баланс!";
EXPECT_THROW(acc.ChangeBalance(100), std::runtime_error)<<"Отсутствует исключение при отсутствии блокировки!"; //at first lock the account
EXPECT_NO_THROW(acc.Lock())<<"Вызывается ненужное исключение при acc.Lock()";
EXPECT_THROW(acc.Lock(), std::runtime_error)<<"Отстуствует исключение при поаторной блокировке!"; //already locked
acc.ChangeBalance(100);
EXPECT_EQ(acc.GetBalance(), 150)<<"Неверно работает ChangeBalance!"; //50+100
}

TEST(Transaction, Test){
Transaction tran;
Account acc1(1, 150);
Account acc2(2, 250);
tran.set_fee(51);
EXPECT_EQ(tran.fee(), 51)<<"Неверно установливается fee!";
EXPECT_THROW(tran.Make(acc1, acc1, 50), std::logic_error)<<"Отсутсвует исключение из-за перевода самому себе!"; //invalid action
EXPECT_THROW(tran.Make(acc1, acc2, -5), std::invalid_argument)<<"Отсутсвует исключение из-за отрицательной суммы!"; //sum can't be negative
EXPECT_THROW(tran.Make(acc1, acc2, 99), std::logic_error)<<"Отсутсвует исключение (sum<100)!"; //too small
EXPECT_FALSE(tran.Make(acc1, acc2, 100))<<"Выполныется перевод, несмотря на слишком большую коммисию!"; //fee*2>100
EXPECT_FALSE(tran.Make(acc1, acc2, 150))<<"Выполняется перевод, несмотря на превышение макс порога!"; //fee+sum>acc1.GetBalance()
EXPECT_TRUE(tran.Make(acc2, acc1, 150))<<"Не выполняется перевод!";
}

TEST(Transaction, CorrectMock){
using ::testing::InSequence;
using ::testing::Return;

Transaction tran;
tran.set_fee(50);
AccountMock acc1(1, 250);
AccountMock acc2(2, 350);

{
InSequence seq;

//Конструктор Guard
EXPECT_CALL(acc1, Lock());
EXPECT_CALL(acc2, Lock());
//Credit
EXPECT_CALL(acc2, ChangeBalance(100));
//Debit
EXPECT_CALL(acc1, GetBalance()).WillOnce(Return(250)); //account.GetBalance()>sum
EXPECT_CALL(acc1, ChangeBalance(-150)); //100+fee
//SaveToDataBase
EXPECT_CALL(acc1, GetBalance()).WillOnce(Return(100));
EXPECT_CALL(acc2, GetBalance()).WillOnce(Return(450));
//Деструктор Guard
EXPECT_CALL(acc2, Unlock());
EXPECT_CALL(acc1, Unlock());
//Сама транзакция
EXPECT_TRUE(tran.Make(acc1, acc2, 100));//<<"Транзакция возвращает неверное значение!";

}
}

TEST(Transaction, UncorrectMock){
using ::testing::InSequence;
using ::testing::Return;

Transaction tran;
tran.set_fee(50);
AccountMock acc1(1, 250);
AccountMock acc2(2, 350);

{
InSequence seq;

//Конструктор Guard
EXPECT_CALL(acc1, Lock());
EXPECT_CALL(acc2, Lock());
//Credit
EXPECT_CALL(acc2, ChangeBalance(200));
//Debit
EXPECT_CALL(acc1, GetBalance()); //account.GetBalance()>sum
//Условие не выполнено (250!>200+fee)
//Возврат переведённой суммы
EXPECT_CALL(acc2, ChangeBalance(-200));
//SaveToDataBase
EXPECT_CALL(acc1, GetBalance()).WillOnce(Return(100));
EXPECT_CALL(acc2, GetBalance()).WillOnce(Return(450));
//Деструктор Guard
EXPECT_CALL(acc2, Unlock());
EXPECT_CALL(acc1, Unlock());
//Сама транзакция
EXPECT_FALSE(tran.Make(acc1, acc2, 200))<<"Транзакция возвращает неверное значение!";

}
}

