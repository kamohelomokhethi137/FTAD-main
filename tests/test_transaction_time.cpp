
// Tests for parseTransactionTime().
//
// This helper exists because building a local-time QDateTime dominated
// the analysis runtime. The tests pin down both the format it accepts
// and the arithmetic the detector relies on, so the optimisation cannot
// silently change which timestamps are considered valid.

#include <gtest/gtest.h>

#include <QDateTime>
#include <QString>

#include "TransactionTime.h"

TEST(TransactionTime, ParsesTheStoredFormat)
{
    const QDateTime parsed =
        parseTransactionTime("2026-10-30 09:15:00");

    ASSERT_TRUE(parsed.isValid());
    EXPECT_EQ(parsed.date(), QDate(2026, 10, 30));
    EXPECT_EQ(parsed.time(), QTime(9, 15, 0));
}

TEST(TransactionTime, ParsesMidnightAndEndOfYear)
{
    const QDateTime midnight =
        parseTransactionTime("2026-01-01 00:00:00");

    ASSERT_TRUE(midnight.isValid());
    EXPECT_EQ(midnight.date(), QDate(2026, 1, 1));
    EXPECT_EQ(midnight.time(), QTime(0, 0, 0));

    const QDateTime last =
        parseTransactionTime("2026-12-31 23:59:59");

    ASSERT_TRUE(last.isValid());
    EXPECT_EQ(last.date(), QDate(2026, 12, 31));
    EXPECT_EQ(last.time(), QTime(23, 59, 59));
}

TEST(TransactionTime, HandlesALeapDay)
{
    const QDateTime leap =
        parseTransactionTime("2028-02-29 12:00:00");

    ASSERT_TRUE(leap.isValid());
    EXPECT_EQ(leap.date(), QDate(2028, 2, 29));
}

TEST(TransactionTime, RejectsNonLeapYearFebruary29)
{
    EXPECT_FALSE(
        parseTransactionTime("2026-02-29 12:00:00").isValid());
}

TEST(TransactionTime, RejectsWrongLengthOrSeparator)
{
    EXPECT_FALSE(parseTransactionTime("").isValid());
    EXPECT_FALSE(parseTransactionTime("2026-10-30").isValid());
    EXPECT_FALSE(
        parseTransactionTime("2026-10-30T09:15:00").isValid());
    EXPECT_FALSE(
        parseTransactionTime("2026-10-30 09:15:00 ").isValid());
    EXPECT_FALSE(
        parseTransactionTime("2026-10-30 09:15:00.000").isValid());
}

TEST(TransactionTime, RejectsImpossibleFields)
{
    EXPECT_FALSE(
        parseTransactionTime("2026-13-30 09:15:00").isValid());
    EXPECT_FALSE(
        parseTransactionTime("2026-10-32 09:15:00").isValid());
    EXPECT_FALSE(
        parseTransactionTime("2026-10-30 25:15:00").isValid());
    EXPECT_FALSE(
        parseTransactionTime("2026-10-30 09:61:00").isValid());
}

TEST(TransactionTime, RejectsFreeText)
{
    EXPECT_FALSE(parseTransactionTime("not-a-date").isValid());
    EXPECT_FALSE(
        parseTransactionTime("30/10/2026 10:30:00").isValid());
}

TEST(TransactionTime, OrdersChronologically)
{
    const QDateTime earlier =
        parseTransactionTime("2026-10-30 09:00:00");
    const QDateTime later =
        parseTransactionTime("2026-10-30 09:05:00");

    ASSERT_TRUE(earlier.isValid());
    ASSERT_TRUE(later.isValid());

    EXPECT_LT(earlier, later);
    EXPECT_GT(later, earlier);
    EXPECT_EQ(earlier, parseTransactionTime("2026-10-30 09:00:00"));
}

TEST(TransactionTime, ComparesAcrossDayAndMonthBoundaries)
{
    EXPECT_LT(parseTransactionTime("2026-10-31 23:59:59"),
              parseTransactionTime("2026-11-01 00:00:00"));

    EXPECT_LT(parseTransactionTime("2026-12-31 23:59:59"),
              parseTransactionTime("2027-01-01 00:00:00"));
}

TEST(TransactionTime, WindowArithmeticCrossesMidnight)
{
    // The duplicate rule offsets by minutes, which must roll the date
    // forward rather than wrap the time of day.
    const QDateTime before =
        parseTransactionTime("2026-10-30 23:58:00").addSecs(300);

    EXPECT_EQ(before.date(), QDate(2026, 10, 31));
    EXPECT_EQ(before.time(), QTime(0, 3, 0));

    const QDateTime after =
        parseTransactionTime("2026-11-01 00:02:00").addSecs(-300);

    EXPECT_EQ(after.date(), QDate(2026, 10, 31));
    EXPECT_EQ(after.time(), QTime(23, 57, 0));
}

TEST(TransactionTime, DifferenceIsExactInSeconds)
{
    const QDateTime start =
        parseTransactionTime("2026-10-30 09:00:00");
    const QDateTime end =
        parseTransactionTime("2026-10-30 09:45:30");

    ASSERT_TRUE(start.isValid());
    ASSERT_TRUE(end.isValid());

    EXPECT_EQ(start.secsTo(end), 45 * 60 + 30);
}

TEST(TransactionTime, IsUnaffectedByDaylightSavingTransitions)
{
    // The value is an absolute instant, so a window that spans a DST
    // change is still measured in real elapsed seconds.
    const QDateTime before =
        parseTransactionTime("2026-03-29 01:59:00");
    const QDateTime after =
        parseTransactionTime("2026-03-29 03:01:00");

    ASSERT_TRUE(before.isValid());
    ASSERT_TRUE(after.isValid());

    EXPECT_EQ(before.secsTo(after), 62 * 60);
}

TEST(TransactionTime, ReportsUtcSpec)
{
    const QDateTime parsed =
        parseTransactionTime("2026-10-30 09:15:00");

    ASSERT_TRUE(parsed.isValid());
    EXPECT_EQ(parsed.timeSpec(), Qt::UTC);
}
