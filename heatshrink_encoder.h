#ifndef HEATSHRINK_ENCODER_H
#define HEATSHRINK_ENCODER_H

#include <stdint.h>
#include <stddef.h>
#include "heatshrink_common.h"

#if CONFIG_ZEPHYR_HEATSHRINK_MODULE
#include "zephyr/heatshrink_config.h"
#else
#include "heatshrink_config.h"
#endif

/**
 * @file heatshrink_encoder.h
 *
 * @brief Heatshrink Encoder API
 *
 * This header file defines the API for the Heatshrink encoder library.
 */

/**
 * @brief Sink result codes.
 *
 * These are the possible return values for the `heatshrink_encoder_sink` function.
 */
typedef enum {
    HSER_SINK_OK,               /**< Data successfully sunk into the input buffer. */
    HSER_SINK_ERROR_NULL = -1,  /**< NULL argument error. */
    HSER_SINK_ERROR_MISUSE = -2 /**< API misuse error. */
} HSE_sink_res;

/**
 * @brief Poll result codes.
 *
 * These are the possible return values for the `heatshrink_encoder_poll` function.
 */
typedef enum {
    HSER_POLL_EMPTY,            /**< Input exhausted. */
    HSER_POLL_MORE,             /**< Poll again for more output. */
    HSER_POLL_ERROR_NULL = -1,  /**< NULL argument error. */
    HSER_POLL_ERROR_MISUSE = -2 /**< API misuse error. */
} HSE_poll_res;

/**
 * @brief Finish result codes.
 *
 * These are the possible return values for the `heatshrink_encoder_finish` function.
 */
typedef enum {
    HSER_FINISH_DONE,           /**< Encoding is complete. */
    HSER_FINISH_MORE,           /**< More output remaining; use poll. */
    HSER_FINISH_ERROR_NULL = -1 /**< NULL argument error. */
} HSE_finish_res;

#if HEATSHRINK_DYNAMIC_ALLOC
#define HEATSHRINK_ENCODER_WINDOW_BITS(HSE) \
    ((HSE)->window_sz2)
#define HEATSHRINK_ENCODER_LOOKAHEAD_BITS(HSE) \
    ((HSE)->lookahead_sz2)
#define HEATSHRINK_ENCODER_INDEX(HSE) \
    ((HSE)->search_index)
struct hs_index {
    uint16_t size;
    int16_t index[];
};
#else
#define HEATSHRINK_ENCODER_WINDOW_BITS(_) \
    (HEATSHRINK_STATIC_WINDOW_BITS)
#define HEATSHRINK_ENCODER_LOOKAHEAD_BITS(_) \
    (HEATSHRINK_STATIC_LOOKAHEAD_BITS)
#define HEATSHRINK_ENCODER_INDEX(HSE) \
    (&(HSE)->search_index)
struct hs_index {
    uint16_t size;
    int16_t index[2 << HEATSHRINK_STATIC_WINDOW_BITS];
};
#endif

/**
 * @brief Heatshrink encoder structure.
 *
 * This structure represents a Heatshrink encoder. It contains various fields
 * necessary for the encoding process.
 */
typedef struct {
    uint16_t input_size;        /**< Bytes in the input buffer. */
    uint16_t match_scan_index;
    uint16_t match_length;
    uint16_t match_pos;
    uint16_t outgoing_bits;     /**< Enqueued outgoing bits. */
    uint8_t outgoing_bits_count;
    uint8_t flags;
    uint8_t state;              /**< Current state machine node. */
    uint8_t current_byte;       /**< Current byte of output. */
    uint8_t bit_index;          /**< Current bit index. */

#if HEATSHRINK_DYNAMIC_ALLOC
    uint8_t window_sz2;         /**< 2^n size of window. */
    uint8_t lookahead_sz2;      /**< 2^n size of lookahead. */
#if HEATSHRINK_USE_INDEX
    struct hs_index *search_index;
#endif
    /* Input buffer and sliding window for expansion. */
    uint8_t buffer[];
#else
    #if HEATSHRINK_USE_INDEX
        struct hs_index search_index;
    #endif
    /* Input buffer and sliding window for expansion. */
    uint8_t buffer[2 << HEATSHRINK_ENCODER_WINDOW_BITS(_)];
#endif
} heatshrink_encoder;

#if HEATSHRINK_DYNAMIC_ALLOC
/**
 * @brief Allocate a Heatshrink encoder.
 * @note Only for dynamic allocation mode
 *
 * Allocate a new encoder struct with specified window and lookahead sizes.
 *
 * @param window_sz2 Size of the window (2^n).
 * @param lookahead_sz2 Size of the lookahead (2^n).
 *
 * @return Pointer to the allocated encoder, or NULL on error.
 */
heatshrink_encoder *heatshrink_encoder_alloc(uint8_t window_sz2, uint8_t lookahead_sz2);

/**
 * @brief Free a Heatshrink encoder.
 * @note Only for dynamic allocation mode
 *
 * Free the memory associated with an encoder.
 *
 * @param hse Pointer to the encoder to be freed.
 */
void heatshrink_encoder_free(heatshrink_encoder *hse);
#endif /* HEATSHRINK_DYNAMIC_ALLOC */

/**
 * @brief Reset a Heatshrink encoder.
 *
 * Reset an encoder to its initial state.
 *
 * @param hse Pointer to the encoder to be reset.
 */
void heatshrink_encoder_reset(heatshrink_encoder *hse);

/**
 * @brief Sink data into the encoder.
 *
 * Sink up to SIZE bytes from IN_BUF into the encoder. The INPUT_SIZE is set to
 * indicate how many bytes were actually sunk (in case a buffer was filled).
 *
 * @param hse Pointer to the encoder.
 * @param in_buf Pointer to the input buffer.
 * @param size Size of the input data.
 * @param input_size Pointer to the variable indicating the number of bytes actually sunk.
 *
 * @return HSE_sink_res value indicating the result of the operation.
 */
HSE_sink_res heatshrink_encoder_sink(heatshrink_encoder *hse, uint8_t *in_buf, size_t size, size_t *input_size);

/**
 * @brief Poll for output from the encoder.
 *
 * Poll for output from the encoder, copying at most OUT_BUF_SIZE bytes into
 * OUT_BUF (setting OUTPUT_SIZE to the actual amount copied).
 *
 * @param hse Pointer to the encoder.
 * @param out_buf Pointer to the output buffer.
 * @param out_buf_size Size of the output buffer.
 * @param output_size Pointer to the variable indicating the number of bytes actually output.
 *
 * @return HSE_poll_res value indicating the result of the operation.
 */
HSE_poll_res heatshrink_encoder_poll(heatshrink_encoder *hse, uint8_t *out_buf, size_t out_buf_size, size_t *output_size);

/**
 * @brief Notify the encoder that the input stream is finished.
 *
 * Notify the encoder that the input stream is finished. If the return value is
 * HSER_FINISH_MORE, there is still more output, so call `heatshrink_encoder_poll` and repeat.
 *
 * @param hse Pointer to the encoder.
 *
 * @return HSE_finish_res value indicating the result of the operation.
 */
HSE_finish_res heatshrink_encoder_finish(heatshrink_encoder *hse);

#endif /* HEATSHRINK_ENCODER_H */
